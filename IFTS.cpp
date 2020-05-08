#include "stdafx.h"
#include "IFTS.h"
#include "mini.h"
#include <fstream>

CIFTSResp::CIFTSResp(void)
{
    LoadConfig();
}

CIFTSResp::~CIFTSResp(void)
{

}

bool CIFTSResp::MakeRespPkg(CMsgInfo& oReq, CMsgInfo& oResp)
{
    bool bRet = true;

    // ���������
    string strXml(oReq.psPkg, oReq.iPkgLen);
    LOGD("��������:%s", strXml.c_str());
    if(!oReq.oPkg.Parse(strXml))
    {
        LOGE("XML����ʧ��", strXml.c_str());
        return false;
    }

    // ��ȡӦ�����
    oResp = oReq;
    oResp.oPkg.Clear();
    oResp.oPkg.m_mapRootAttr = oReq.oPkg.m_mapRootAttr;
    oResp.oPkg.m_strMsgType = oReq.oPkg.GetRetMsgType();
    CTxMsg& oMsg = oReq.oPkg;
    // ͬʱ�ж������Ӧ����Ϊ�˼���ĳЩ�����ĸ�ʽ����
    if(oMsg.m_strMsgType==MSG_TYPE_SESSION || oMsg.m_strMsgType==MSG_TYPE_SESSION_R)
    {
        bRet = MakeRespPkgSession(oReq, oResp);
    }
    else if(oMsg.m_strMsgType==MSG_TYPE_OPEN_ACCT || oMsg.m_strMsgType==MSG_TYPE_OPEN_ACCT_R)
    {
        bRet = MakeRespPkgOpenAcct(oReq, oResp);
    }
    else if(oMsg.m_strMsgType==MSG_TYPE_TRADE || oMsg.m_strMsgType==MSG_TYPE_TRADE_R)
    {
        bRet = MakeRespPkgTrade(oReq, oResp);
    }
    else
    {
        LOGE("��֧�ֵ���Ϣ����[%s]", oMsg.m_strMsgType.c_str());
        return false;
    }

    // ����Ӧ�����
    string strRespXml = oResp.oPkg.PackXml();
    char* pRetXml = new char[strRespXml.size() + 1];
    oResp.psPkg = pRetXml;
    memset(pRetXml, 0, strRespXml.size() + 1);
    memcpy(pRetXml, strRespXml.c_str(), strRespXml.size());
    oResp.iPkgLen = strRespXml.size();
    LOGD("ģ��Ӧ����:%s", strRespXml.c_str());

    return bRet;
}

void CIFTSResp::LoadConfig()
{
    CSimpleIni ini;
    ini.SetUnicode();
    ini.LoadFile("mrapi_conf.ini");
    m_strRetCode = ini.GetValue("AutoRet", "RetCode", "000");
    m_strRetInfo = ini.GetValue("AutoRet", "RetInfo", "ģ��ɹ�");
    m_BankReqFile = ini.GetValue("AutoRet", "BankReqFile", "BankReqFile.log");
    m_BankAnsFile = ini.GetValue("AutoRet", "BankAnsFile", "BankAnsFile.log");
}

bool CIFTSResp::MakeRespPkgSession(CMsgInfo& oReq, CMsgInfo& oResp)
{
    // 1		��Ϣͷ	MessageHeader	<MsgHdr>	[1..1]	MessageHeader ���
    // 2		���ؽ��	ReturnResult	<Rst>	[1..1]	ReturnResult ���
    // Ӧ����Ϣ
    CTxMsg& oAmsg = oResp.oPkg;

    // ������Ϣ����
    CTxMsg& oRmsg = oReq.oPkg;
    // 1.��Ϣͷ
    CMessgeHeader* pRHdr = dynamic_cast<CMessgeHeader*>(oRmsg.GetComponent("MsgHdr"));
    if(!pRHdr)
    {
        LOGE("��ȡ��Ϣͷ��ǩ<MsgHdr>ʧ��");
        return false;
    }
    CMessgeHeader* pAHdr = new CMessgeHeader(*pRHdr);
    pAHdr->strCreateDate = MGetCurDate("%Y%m%d");
    pAHdr->strCreateTime = MGetCurDate("%H:%M:%S");
    pAHdr->oRelatedReference = pRHdr->oRefrence;
    pAHdr->oRefrence.strReference = MGetLsh();
    // ȡ�����෴��
    string strInstType = "S";
    if(!pRHdr->oSender.strInstitutionType.empty())
    {
        strInstType = pRHdr->oSender.strInstitutionType;
    }
    // ȡ��ˮ����Ϣ
    else if(!pRHdr->oRefrence.strRefrenceIssureType.empty())
    {
        strInstType = pRHdr->oRefrence.strRefrenceIssureType;
    }
    // ȡ��ͷ
    else if(oAmsg.m_mapRootAttr.find("Type") != oAmsg.m_mapRootAttr.end())
    {
        strInstType = oAmsg.m_mapRootAttr["Type"];
    }
    pAHdr->oRefrence.strRefrenceIssureType = 'B' + 'S' - (strInstType.empty() ? 'B':strInstType[0]);
    // ���ͷ��ͽ��շ�ȡ��
    swap(pAHdr->oSender, pAHdr->oRecver);
    oAmsg.AddComponent("MsgHdr", pAHdr);

    // 2.������Ϣ
    CReturnResult* pRst = new CReturnResult();
    pRst->strCode = m_strRetCode;
    pRst->strInfo = m_strRetInfo;
    oAmsg.AddComponent("Rst", pRst);

    return true;
}

bool CIFTSResp::MakeRespPkgTrade(CMsgInfo& oReq, CMsgInfo& oResp)
{
    // 1	��Ϣͷ	MessageHeader	<MsgHdr>	[1..1]	MessageHeader ���
    // 2	���ؽ��	ReturnResult	<Rst>	[1..1]	ReturnResult ���
    // 3	���з��˻�	BankAccount	<BkAcct>	[0..1]	Account ���
    // 4	֤ȯ���˻�	SecuritiesAccount	<ScAcct>	[0..1]	Account ���
    // 5	����	Currency	<Ccy>	[0..1]	CurrencyCode
    // 6	ת�˽��	TransferAmout	<TrfAmt>	[0..1]	Amount
    // 7	ժҪ	Digest	<Dgst>	[0..1]	Max35Text

    // Ӧ����Ϣ
    CTxMsg& oAmsg = oResp.oPkg;

    // ������Ϣ����
    CTxMsg& oRmsg = oReq.oPkg;
    // 1.��Ϣͷ
    CMessgeHeader* pRHdr = dynamic_cast<CMessgeHeader*>(oRmsg.GetComponent("MsgHdr"));
    if(!pRHdr)
    {
        LOGE("��ȡ��Ϣͷ��ǩ<MsgHdr>ʧ��");
        return false;
    }
    CMessgeHeader* pAHdr = new CMessgeHeader(*pRHdr);
    pAHdr->strCreateDate = MGetCurDate("%Y%m%d");
    pAHdr->strCreateTime = MGetCurDate("%H:%M:%S");
    pAHdr->oRelatedReference = pRHdr->oRefrence;
    pAHdr->oRefrence.strReference = MGetLsh();
    // ȡ�����෴��
    string strInstType = "S";
    if(!pRHdr->oSender.strInstitutionType.empty())
    {
        strInstType = pRHdr->oSender.strInstitutionType;
    }
    // ȡ��ˮ����Ϣ
    else if(!pRHdr->oRefrence.strRefrenceIssureType.empty())
    {
        strInstType = pRHdr->oRefrence.strRefrenceIssureType;
    }
    // ȡ��ͷ
    else if(oAmsg.m_mapRootAttr.find("Type") != oAmsg.m_mapRootAttr.end())
    {
        strInstType = oAmsg.m_mapRootAttr["Type"];
    }
    pAHdr->oRefrence.strRefrenceIssureType = 'B' + 'S' - (strInstType.empty() ? 'B':strInstType[0]);
    // ���ͷ��ͽ��շ�ȡ��
    swap(pAHdr->oSender, pAHdr->oRecver);
    oAmsg.AddComponent("MsgHdr", pAHdr);

    // 2.������Ϣ
    CReturnResult* pRst = new CReturnResult();
    pRst->strCode = m_strRetCode;
    pRst->strInfo = m_strRetInfo;
    oAmsg.AddComponent("Rst", pRst);

    // 4.֤ȯ���˻���Ϣ
    CAccount* pRScAcct = dynamic_cast<CAccount*>(oRmsg.GetComponent("ScAcct"));
    if(pRScAcct)
    {
        CAccount* pAScAcct = new CAccount(*pRScAcct);
        oAmsg.AddComponent("ScAcct", pAScAcct);
    }

    // 3.���з��˻���Ϣ
    CAccount* pRBkAcct = dynamic_cast<CAccount*>(oRmsg.GetComponent("BkAcct"));
    if(pRBkAcct)
    {
        CAccount* pABkAcct = new CAccount(*pRBkAcct);
        // �������п���
        if(pRScAcct && pABkAcct->strAccountIdentification.empty())
        {
            pABkAcct->strAccountIdentification = MMakeBkAcct(pRScAcct->strAccountIdentification, pRBkAcct->strAccountIdentification);
        }
        oAmsg.AddComponent("BkAcct", pABkAcct);
    }

    // 5.����
    CMsgComponent* pRCcy = oRmsg.GetComponent("Ccy");
    if(pRCcy)
    {
        CMsgComponent* pACcy = new CMsgComponent(*pRCcy);
        oAmsg.AddComponent("Ccy", pACcy);
    }
    // 6.ת�˽��
    CMsgComponent* pRTrfAmt = oRmsg.GetComponent("TrfAmt");
    if(pRTrfAmt)
    {
        CMsgComponent* pATrfAmt = new CMsgComponent(*pRTrfAmt);
        oAmsg.AddComponent("CashExCode", pATrfAmt);
    }
    // 7.ժҪ
    CMsgComponent* pRDgst = oRmsg.GetComponent("Dgst");
    if(pRDgst)
    {
        CMsgComponent* pADgst = new CMsgComponent(*pRDgst);
        oAmsg.AddComponent("Dgst", pADgst);
    }

    return true;
}

bool CIFTSResp::MakeRespPkgOpenAcct(CMsgInfo& oReq, CMsgInfo& oResp)
{
    // 1	��Ϣͷ	MessageHeader	<MsgHdr>	[1..1]	MessageHeader
    // 2	���ؽ��	ReturnResult	<Rst>	[1..1]	ReturnResult
    // 3	���з��˻�	BankAccount	<BkAcct>	[0..1]	Account
    // 4	֤ȯ���˻�	SecuritiesAccount	<ScAcct>	[0..1]	Account
    // 5	����	Currency	<Ccy>	[0..1]	CurrencyCode
    // 6	�㳮��־	CashExCode	<CashExCd>	[0..1]	CashExCode
    // 7	֤ȯ���˻����	SecuritiesBalance	<ScBal>	[0..1]	Balance
    // 8	ժҪ	Digest	<Dgst>	[0..1]	Max35Text

    // Ӧ����Ϣ
    CTxMsg& oAmsg = oResp.oPkg;

    // ������Ϣ����
    CTxMsg& oRmsg = oReq.oPkg;
    // 1.��Ϣͷ
    CMessgeHeader* pRHdr = dynamic_cast<CMessgeHeader*>(oRmsg.GetComponent("MsgHdr"));
    if(!pRHdr)
    {
        LOGE("��ȡ��Ϣͷ��ǩ<MsgHdr>ʧ��");
        return false;
    }
    CMessgeHeader* pAHdr = new CMessgeHeader(*pRHdr);
    pAHdr->strCreateDate = MGetCurDate("%Y%m%d");
    pAHdr->strCreateTime = MGetCurDate("%H:%M:%S");
    pAHdr->oRelatedReference = pRHdr->oRefrence;
    pAHdr->oRefrence.strReference = MGetLsh();
    // ȡ�����෴��
    string strInstType = "S";
    if(!pRHdr->oSender.strInstitutionType.empty())
    {
        strInstType = pRHdr->oSender.strInstitutionType;
    }
    // ȡ��ˮ����Ϣ
    else if(!pRHdr->oRefrence.strRefrenceIssureType.empty())
    {
        strInstType = pRHdr->oRefrence.strRefrenceIssureType;
    }
    // ȡ��ͷ
    else if(oAmsg.m_mapRootAttr.find("Type") != oAmsg.m_mapRootAttr.end())
    {
        strInstType = oAmsg.m_mapRootAttr["Type"];
    }
    pAHdr->oRefrence.strRefrenceIssureType = 'B' + 'S' - (strInstType.empty() ? 'B':strInstType[0]);
    // ���ͷ��ͽ��շ�ȡ��
    swap(pAHdr->oSender, pAHdr->oRecver);
    oAmsg.AddComponent("MsgHdr", pAHdr);

    // 2.������Ϣ
    CReturnResult* pRst = new CReturnResult();
    pRst->strCode = m_strRetCode;
    pRst->strInfo = m_strRetInfo;
    oAmsg.AddComponent("Rst", pRst);

    // 4.֤ȯ���˻���Ϣ
    CAccount* pRScAcct = dynamic_cast<CAccount*>(oRmsg.GetComponent("ScAcct"));
    if(pRScAcct)
    {
        CAccount* pAScAcct = new CAccount(*pRScAcct);
        oAmsg.AddComponent("ScAcct", pAScAcct);
    }

    // 3.���з��˻���Ϣ
    CAccount* pRBkAcct = dynamic_cast<CAccount*>(oRmsg.GetComponent("BkAcct"));
    if(pRBkAcct)
    {
        CAccount* pABkAcct = new CAccount(*pRBkAcct);
        // �������п���
        if(pRScAcct && pABkAcct->strAccountIdentification.empty())
        {
            pABkAcct->strAccountIdentification = MMakeBkAcct(pRScAcct->strAccountIdentification, pRBkAcct->strAccountIdentification);
        }
        oAmsg.AddComponent("BkAcct", pABkAcct);
    }

    // 5.����
    CMsgComponent* pRCcy = oRmsg.GetComponent("Ccy");
    if(pRCcy)
    {
        CMsgComponent* pACcy = new CMsgComponent(*pRCcy);
        oAmsg.AddComponent("Ccy", pACcy);
    }
    // 6.�㳮��־
    CMsgComponent* pRCashExCd = oRmsg.GetComponent("CashExCode");
    if(pRCashExCd)
    {
        CMsgComponent* pACashExCd = new CMsgComponent(*pRCashExCd);
        oAmsg.AddComponent("CashExCode", pACashExCd);
    }
    // 7.֤ȯ���˻����
    CBalance* pRScBal = dynamic_cast<CBalance*>(oRmsg.GetComponent("ScBal"));
    if(pRScBal)
    {
        CMsgComponent* pAScBal = new CMsgComponent(*pRScBal);
        oAmsg.AddComponent("ScBal", pAScBal);
    }
    // 8.ժҪ
    CMsgComponent* pRDgst = oRmsg.GetComponent("Dgst");
    if(pRDgst)
    {
        CMsgComponent* pADgst = new CMsgComponent(*pRDgst);
        oAmsg.AddComponent("Dgst", pADgst);
    }

    return true;
}

bool CMsgComponent::Parse(const string& strXml)
{
    // ����XML����
    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_SUCCESS != doc.Parse(strXml.c_str()))
    {
        LOGE("XML�������ɹ���XMLDocument.Parse");
        return false;
    }
    return Parse(doc.RootElement());
}

bool CMsgComponent::Parse(const tinyxml2::XMLElement* pe)
{
    _map_val.clear();
    if(pe == NULL) return false;
    for (const tinyxml2::XMLElement* ce = pe->FirstChildElement(); ce; ce = ce->NextSiblingElement())
    {
        if (ce->GetText() != NULL)
        {
            _map_val[ce->Name()] = ce->GetText();
        }
    }
    return true;
}

std::string MGetCurDate(const char *format)
{
    time_t tCur;
    struct tm* tmCur;
    char szBuf[128] = {0};

    time(&tCur);

    tmCur = localtime(&tCur);
    strftime(szBuf, sizeof(szBuf)-1, format, tmCur);
    return szBuf;
}

std::string MGetLsh()
{
    static time_t tNow = 0;
    if(tNow == 0)
    {
        time(&tNow);
        tNow = tNow%100000000;
    }
    char buf[32] = {0};
    sprintf(buf, "%ld", ++tNow);
    return buf;
}

std::string MMakeBkAcct(const string& strScAcct, const string& strBkAcct)
{
    if(strBkAcct.empty())
    {
        return "5405-" + strScAcct;
    }
    return strBkAcct;
}

std::string MMakeXmlElem(const string& strTag, const string& strText)
{
    return "<" + strTag + ">" + strText + "</" + strTag + ">\n";
}

int MCalCheckSum(const string& strBody)
{
    int sum = 0;
    for (size_t i=0; i<strBody.size(); i++)
    {
        sum += strBody[i];
        if(sum > 102400)
        {
            sum %= 256;
        }
    }
    return sum%256;
}

const tinyxml2::XMLElement* XmlFindFirst(const tinyxml2::XMLElement* pe, const string& strName)
{
    if(!pe) return pe;
    return pe->FirstChildElement(strName.c_str());
}

std::string XmlTextFirst(const tinyxml2::XMLElement* pe, const string& strName)
{
    const tinyxml2::XMLElement* p = XmlFindFirst(pe, strName);
    if(p && p->GetText())
    {
        return p->GetText();
    }
    return "";
}

CTxMsg::~CTxMsg()
{
    Clear();
}

bool CTxMsg::Parse(tinyxml2::XMLElement* pr)
{
    if(!pr) return false;
    // �������
    Clear();

    // ��ȡ���ڵ�����ֵ
    for (const tinyxml2::XMLAttribute* pa = pr->FirstAttribute(); pa; pa=pa->Next())
    {
        m_mapRootAttr[pa->Name()] = pa->Value();
    }

    // ��ȡ��Ϣ����
    tinyxml2::XMLElement* pMsgBody = pr->FirstChildElement()->FirstChildElement();
    m_strMsgType = pMsgBody->Name();
    if(!ParseComponents(pMsgBody))
    {
        LOGE("�������ʧ��");
        return false;
    }

    return true;
}

bool CTxMsg::Parse(const string& strXml)
{
    // ��������
    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_SUCCESS != doc.Parse(strXml.c_str()))
    {
        LOGE("XML�������ɹ���XMLDocument.Parse");
        return false;
    }

    // ��ȡ���ڵ�����ֵ
    tinyxml2::XMLElement* pr = doc.RootElement();
    return Parse(pr);
}

std::string CTxMsg::PackXml()
{
    // ����
    string strBody;
    for (map<string, CMsgComponent*>::iterator i=m_mapComponent.begin(); i!=m_mapComponent.end(); ++i)
    {
        strBody += MMakeXmlElem(i->first, i->second->PackXml());
    }

    // �Ӱ�����
    strBody = MMakeXmlElem(m_strMsgType, "\n" + strBody);
    strBody = MMakeXmlElem("MsgText", "\n" + strBody);

    // ����У��ֵ
    char buf[128] = {0};
    int nCheckSum = MCalCheckSum(strBody);
    sprintf(buf, "%d", nCheckSum);
    m_mapRootAttr["CheckSum"] = buf;
    // �������
    sprintf(buf, "%05d", static_cast<int>(strBody.size()));
    m_mapRootAttr["Len"] = buf;

    // ��ͷ���Գ���
    size_t nAttrLen = 0;
    for (map<string, string>::iterator i=m_mapRootAttr.begin(); i!=m_mapRootAttr.end(); ++i)
    {
        nAttrLen += i->first.size() + i->second.size() + 4;
    }

    // �ܰ�����
    string pXmlSec[] = {"<IFTS", ">\n", "</IFTS>", ""};
    size_t nLen = strBody.size() + nAttrLen;
    for(int i=0; !pXmlSec[i].empty(); ++i)
    {
        nLen += pXmlSec[i].size();
    }
    sprintf(buf, "%05d", static_cast<int>(nLen));
    m_mapRootAttr["Len"] = buf;

    // ���
    string strXml = pXmlSec[0];
    for (map<string, string>::iterator i=m_mapRootAttr.begin(); i!=m_mapRootAttr.end(); ++i)
    {
        strXml += " " + i->first + "=\"" + i->second + "\"";
    }
    strXml += pXmlSec[1];
    strXml += strBody;
    strXml += pXmlSec[2];

    return strXml;
}

bool CTxMsg::ParseComponents(tinyxml2::XMLElement* pb)
{
    if(!pb) return false;
    // ������Ϣ����Ԫ��
    string strName;
    CMsgComponent* pc = NULL;
    for (const tinyxml2::XMLElement* pe = pb->FirstChildElement(); pe; pe=pe->NextSiblingElement())
    {
        strName = pe->Name();
        // MessgeHeader ['<MsgHdr>']
        if(strName == "MsgHdr")
        {
            pc = new CMessgeHeader();
        }
        // Institution ['<Sender>', '<Recver>', '<AcctSvcr>']
        else if(strName=="Sender" || strName=="Recver" || strName=="AcctSvcr")
        {
            pc = new CInstitution();
        }
        // Reference ['<Ref>', '<RltdRef>']
        else if(strName=="Ref" || strName=="RltdRef")
        {
            pc = new CReference();
        }
        // Password ['<Pwd>']
        else if(strName=="Pwd")
        {
            pc = new CPassword();
        }
        // Customer ['<Cust>']
        else if(strName=="Cust")
        {
            pc = new CCustomer();
        }
        // Account ['<BkAcct>', '<ScAcct>']
        else if(strName=="BkAcct" || strName=="ScAcct")
        {
            pc = new CAccount();
        }
        // ReturnResult ['<ChkRst>']
        else if(strName=="ChkRst")
        {
            pc = new CAccount();
        }
        // Balance ['<ScBal>']
        else if(strName=="ScBal")
        {
            pc = new CBalance();
        }
        else
        {
            pc = new CMsgComponent();
        }
        pc->Parse(pe);
        m_mapComponent[strName] = pc;
    }
    return true;
}

CMsgComponent* CTxMsg::GetComponent(const string& strName)
{
    map<string, CMsgComponent*>::iterator i = m_mapComponent.find(strName);
    if(i != m_mapComponent.end())
    {
        return i->second;
    }
    return NULL;
}

void CTxMsg::AddComponent(const string& strName, CMsgComponent* p)
{
    m_mapComponent[strName] = p;
}

std::string CTxMsg::GetRetMsgType() const
{
    // ��ֳ������е����
    string strType = m_strMsgType;
    size_t b = strType.find_first_of('.');
    size_t e = strType.find_last_of('.');
    if(b==string::npos || e==string::npos)
    {
        return "";
    }
    string strSeq = strType.substr(b+1, 3);
    int nSeq = atoi(strSeq.c_str());

    // ��������ż������ֱ�ӷ���
    if((nSeq&1) == 0)
    {
        return strType;
    }

    // ����Ӧ���������
    char buf[32] = {0};
    sprintf(buf, "%03d", atoi(strSeq.c_str())+1);
    return strType.replace(b+1, 3, buf);
}

bool CTxMsg::IsReqMsg(const string& strXml)
{
    // �����ϢΪӦ���������
    // �ж��Ƿ��з��������Ϣ:<Rst><Code></Code><Info></Info></Rst>
    vector<size_t> vecPos(6, 0);
    vecPos[0] = strXml.find("<Rst>");
    if(vecPos[0] == string::npos) return false;
    vecPos[1] = strXml.find("</Rst>");
    if(vecPos[1] == string::npos) return false;
    vecPos[2] = strXml.find("<Code>");
    if(vecPos[2] == string::npos) return false;
    vecPos[3] = strXml.find("</Code>");
    if(vecPos[3] == string::npos) return false;
    vecPos[4] = strXml.find("<Info>");
    if(vecPos[4] == string::npos) return false;
    vecPos[5] = strXml.find("</Info>");
    if(vecPos[5] == string::npos) return false;

    // ���˳��ΪӦ����Ϣ��ֱ�Ӷ���
    if(vecPos[0]<vecPos[1] &&
        vecPos[2]<vecPos[3] &&
        vecPos[4]<vecPos[5])
    {
        return true;
    }
    return false;
}

int CTxMsg::LoadMsg(const string& strPath, vector<string>& vecXml)
{
    // ���ļ�
    ifstream oFin;
    oFin.open(strPath);
    if(!oFin.is_open())
    {
        return 0;
    }

    // ��ȡ�ļ�
    string strLine;
    string strXml;
    size_t pos = 0;
    while(getline(oFin, strLine))
    {
        // ����ͷ
        if(strXml.empty())
        {
            // �Ƿ���ͷ
            pos = strLine.find("<IFTS");
            if(pos == string::npos)
            {
                continue;
            }
            strXml = strLine.substr(pos);
            // ͬ�д��ڽ�β��־
            pos = strXml.find("</IFTS>");
            if(pos != string::npos)
            {
                strXml = strXml.substr(0, pos+7);
                vecXml.push_back(strXml);
                strXml.clear();
            }
            continue;
        }
        // ����β��
        pos = strLine.find("</IFTS>");
        if(pos != string::npos)
        {
            strXml += strLine.substr(0, pos+7);
            vecXml.push_back(strXml);
            strXml.clear();
            continue;
        }
        strXml += strLine;
    }

    return static_cast<int>(vecXml.size());
}

void CTxMsg::Clear()
{
    // �ڴ�����
    for (map<string, CMsgComponent*>::iterator i=m_mapComponent.begin(); i!=m_mapComponent.end(); ++i)
    {
        delete i->second;
        i->second = NULL;
    }
    m_mapComponent.clear();
    m_mapRootAttr.clear();
    m_strMsgType = "";
}
