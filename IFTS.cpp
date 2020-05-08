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

    // 解析请求包
    string strXml(oReq.psPkg, oReq.iPkgLen);
    LOGD("处理请求:%s", strXml.c_str());
    if(!oReq.oPkg.Parse(strXml))
    {
        LOGE("XML解析失败", strXml.c_str());
        return false;
    }

    // 获取应答包体
    oResp = oReq;
    oResp.oPkg.Clear();
    oResp.oPkg.m_mapRootAttr = oReq.oPkg.m_mapRootAttr;
    oResp.oPkg.m_strMsgType = oReq.oPkg.GetRetMsgType();
    CTxMsg& oMsg = oReq.oPkg;
    // 同时判断请求和应答是为了兼容某些程序报文格式错误
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
        LOGE("不支持的消息类型[%s]", oMsg.m_strMsgType.c_str());
        return false;
    }

    // 生成应答包文
    string strRespXml = oResp.oPkg.PackXml();
    char* pRetXml = new char[strRespXml.size() + 1];
    oResp.psPkg = pRetXml;
    memset(pRetXml, 0, strRespXml.size() + 1);
    memcpy(pRetXml, strRespXml.c_str(), strRespXml.size());
    oResp.iPkgLen = strRespXml.size();
    LOGD("模拟应答报文:%s", strRespXml.c_str());

    return bRet;
}

void CIFTSResp::LoadConfig()
{
    CSimpleIni ini;
    ini.SetUnicode();
    ini.LoadFile("mrapi_conf.ini");
    m_strRetCode = ini.GetValue("AutoRet", "RetCode", "000");
    m_strRetInfo = ini.GetValue("AutoRet", "RetInfo", "模拟成功");
    m_BankReqFile = ini.GetValue("AutoRet", "BankReqFile", "BankReqFile.log");
    m_BankAnsFile = ini.GetValue("AutoRet", "BankAnsFile", "BankAnsFile.log");
}

bool CIFTSResp::MakeRespPkgSession(CMsgInfo& oReq, CMsgInfo& oResp)
{
    // 1		消息头	MessageHeader	<MsgHdr>	[1..1]	MessageHeader 组件
    // 2		返回结果	ReturnResult	<Rst>	[1..1]	ReturnResult 组件
    // 应答消息
    CTxMsg& oAmsg = oResp.oPkg;

    // 请求消息对象
    CTxMsg& oRmsg = oReq.oPkg;
    // 1.消息头
    CMessgeHeader* pRHdr = dynamic_cast<CMessgeHeader*>(oRmsg.GetComponent("MsgHdr"));
    if(!pRHdr)
    {
        LOGE("获取消息头标签<MsgHdr>失败");
        return false;
    }
    CMessgeHeader* pAHdr = new CMessgeHeader(*pRHdr);
    pAHdr->strCreateDate = MGetCurDate("%Y%m%d");
    pAHdr->strCreateTime = MGetCurDate("%H:%M:%S");
    pAHdr->oRelatedReference = pRHdr->oRefrence;
    pAHdr->oRefrence.strReference = MGetLsh();
    // 取机构相反方
    string strInstType = "S";
    if(!pRHdr->oSender.strInstitutionType.empty())
    {
        strInstType = pRHdr->oSender.strInstitutionType;
    }
    // 取流水号信息
    else if(!pRHdr->oRefrence.strRefrenceIssureType.empty())
    {
        strInstType = pRHdr->oRefrence.strRefrenceIssureType;
    }
    // 取包头
    else if(oAmsg.m_mapRootAttr.find("Type") != oAmsg.m_mapRootAttr.end())
    {
        strInstType = oAmsg.m_mapRootAttr["Type"];
    }
    pAHdr->oRefrence.strRefrenceIssureType = 'B' + 'S' - (strInstType.empty() ? 'B':strInstType[0]);
    // 发送方和接收方取反
    swap(pAHdr->oSender, pAHdr->oRecver);
    oAmsg.AddComponent("MsgHdr", pAHdr);

    // 2.返回信息
    CReturnResult* pRst = new CReturnResult();
    pRst->strCode = m_strRetCode;
    pRst->strInfo = m_strRetInfo;
    oAmsg.AddComponent("Rst", pRst);

    return true;
}

bool CIFTSResp::MakeRespPkgTrade(CMsgInfo& oReq, CMsgInfo& oResp)
{
    // 1	消息头	MessageHeader	<MsgHdr>	[1..1]	MessageHeader 组件
    // 2	返回结果	ReturnResult	<Rst>	[1..1]	ReturnResult 组件
    // 3	银行方账户	BankAccount	<BkAcct>	[0..1]	Account 组件
    // 4	证券方账户	SecuritiesAccount	<ScAcct>	[0..1]	Account 组件
    // 5	币种	Currency	<Ccy>	[0..1]	CurrencyCode
    // 6	转账金额	TransferAmout	<TrfAmt>	[0..1]	Amount
    // 7	摘要	Digest	<Dgst>	[0..1]	Max35Text

    // 应答消息
    CTxMsg& oAmsg = oResp.oPkg;

    // 请求消息对象
    CTxMsg& oRmsg = oReq.oPkg;
    // 1.消息头
    CMessgeHeader* pRHdr = dynamic_cast<CMessgeHeader*>(oRmsg.GetComponent("MsgHdr"));
    if(!pRHdr)
    {
        LOGE("获取消息头标签<MsgHdr>失败");
        return false;
    }
    CMessgeHeader* pAHdr = new CMessgeHeader(*pRHdr);
    pAHdr->strCreateDate = MGetCurDate("%Y%m%d");
    pAHdr->strCreateTime = MGetCurDate("%H:%M:%S");
    pAHdr->oRelatedReference = pRHdr->oRefrence;
    pAHdr->oRefrence.strReference = MGetLsh();
    // 取机构相反方
    string strInstType = "S";
    if(!pRHdr->oSender.strInstitutionType.empty())
    {
        strInstType = pRHdr->oSender.strInstitutionType;
    }
    // 取流水号信息
    else if(!pRHdr->oRefrence.strRefrenceIssureType.empty())
    {
        strInstType = pRHdr->oRefrence.strRefrenceIssureType;
    }
    // 取包头
    else if(oAmsg.m_mapRootAttr.find("Type") != oAmsg.m_mapRootAttr.end())
    {
        strInstType = oAmsg.m_mapRootAttr["Type"];
    }
    pAHdr->oRefrence.strRefrenceIssureType = 'B' + 'S' - (strInstType.empty() ? 'B':strInstType[0]);
    // 发送方和接收方取反
    swap(pAHdr->oSender, pAHdr->oRecver);
    oAmsg.AddComponent("MsgHdr", pAHdr);

    // 2.返回信息
    CReturnResult* pRst = new CReturnResult();
    pRst->strCode = m_strRetCode;
    pRst->strInfo = m_strRetInfo;
    oAmsg.AddComponent("Rst", pRst);

    // 4.证券方账户信息
    CAccount* pRScAcct = dynamic_cast<CAccount*>(oRmsg.GetComponent("ScAcct"));
    if(pRScAcct)
    {
        CAccount* pAScAcct = new CAccount(*pRScAcct);
        oAmsg.AddComponent("ScAcct", pAScAcct);
    }

    // 3.银行方账户信息
    CAccount* pRBkAcct = dynamic_cast<CAccount*>(oRmsg.GetComponent("BkAcct"));
    if(pRBkAcct)
    {
        CAccount* pABkAcct = new CAccount(*pRBkAcct);
        // 生成银行卡号
        if(pRScAcct && pABkAcct->strAccountIdentification.empty())
        {
            pABkAcct->strAccountIdentification = MMakeBkAcct(pRScAcct->strAccountIdentification, pRBkAcct->strAccountIdentification);
        }
        oAmsg.AddComponent("BkAcct", pABkAcct);
    }

    // 5.币种
    CMsgComponent* pRCcy = oRmsg.GetComponent("Ccy");
    if(pRCcy)
    {
        CMsgComponent* pACcy = new CMsgComponent(*pRCcy);
        oAmsg.AddComponent("Ccy", pACcy);
    }
    // 6.转账金额
    CMsgComponent* pRTrfAmt = oRmsg.GetComponent("TrfAmt");
    if(pRTrfAmt)
    {
        CMsgComponent* pATrfAmt = new CMsgComponent(*pRTrfAmt);
        oAmsg.AddComponent("CashExCode", pATrfAmt);
    }
    // 7.摘要
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
    // 1	消息头	MessageHeader	<MsgHdr>	[1..1]	MessageHeader
    // 2	返回结果	ReturnResult	<Rst>	[1..1]	ReturnResult
    // 3	银行方账户	BankAccount	<BkAcct>	[0..1]	Account
    // 4	证券方账户	SecuritiesAccount	<ScAcct>	[0..1]	Account
    // 5	币种	Currency	<Ccy>	[0..1]	CurrencyCode
    // 6	汇钞标志	CashExCode	<CashExCd>	[0..1]	CashExCode
    // 7	证券方账户余额	SecuritiesBalance	<ScBal>	[0..1]	Balance
    // 8	摘要	Digest	<Dgst>	[0..1]	Max35Text

    // 应答消息
    CTxMsg& oAmsg = oResp.oPkg;

    // 请求消息对象
    CTxMsg& oRmsg = oReq.oPkg;
    // 1.消息头
    CMessgeHeader* pRHdr = dynamic_cast<CMessgeHeader*>(oRmsg.GetComponent("MsgHdr"));
    if(!pRHdr)
    {
        LOGE("获取消息头标签<MsgHdr>失败");
        return false;
    }
    CMessgeHeader* pAHdr = new CMessgeHeader(*pRHdr);
    pAHdr->strCreateDate = MGetCurDate("%Y%m%d");
    pAHdr->strCreateTime = MGetCurDate("%H:%M:%S");
    pAHdr->oRelatedReference = pRHdr->oRefrence;
    pAHdr->oRefrence.strReference = MGetLsh();
    // 取机构相反方
    string strInstType = "S";
    if(!pRHdr->oSender.strInstitutionType.empty())
    {
        strInstType = pRHdr->oSender.strInstitutionType;
    }
    // 取流水号信息
    else if(!pRHdr->oRefrence.strRefrenceIssureType.empty())
    {
        strInstType = pRHdr->oRefrence.strRefrenceIssureType;
    }
    // 取包头
    else if(oAmsg.m_mapRootAttr.find("Type") != oAmsg.m_mapRootAttr.end())
    {
        strInstType = oAmsg.m_mapRootAttr["Type"];
    }
    pAHdr->oRefrence.strRefrenceIssureType = 'B' + 'S' - (strInstType.empty() ? 'B':strInstType[0]);
    // 发送方和接收方取反
    swap(pAHdr->oSender, pAHdr->oRecver);
    oAmsg.AddComponent("MsgHdr", pAHdr);

    // 2.返回信息
    CReturnResult* pRst = new CReturnResult();
    pRst->strCode = m_strRetCode;
    pRst->strInfo = m_strRetInfo;
    oAmsg.AddComponent("Rst", pRst);

    // 4.证券方账户信息
    CAccount* pRScAcct = dynamic_cast<CAccount*>(oRmsg.GetComponent("ScAcct"));
    if(pRScAcct)
    {
        CAccount* pAScAcct = new CAccount(*pRScAcct);
        oAmsg.AddComponent("ScAcct", pAScAcct);
    }

    // 3.银行方账户信息
    CAccount* pRBkAcct = dynamic_cast<CAccount*>(oRmsg.GetComponent("BkAcct"));
    if(pRBkAcct)
    {
        CAccount* pABkAcct = new CAccount(*pRBkAcct);
        // 生成银行卡号
        if(pRScAcct && pABkAcct->strAccountIdentification.empty())
        {
            pABkAcct->strAccountIdentification = MMakeBkAcct(pRScAcct->strAccountIdentification, pRBkAcct->strAccountIdentification);
        }
        oAmsg.AddComponent("BkAcct", pABkAcct);
    }

    // 5.币种
    CMsgComponent* pRCcy = oRmsg.GetComponent("Ccy");
    if(pRCcy)
    {
        CMsgComponent* pACcy = new CMsgComponent(*pRCcy);
        oAmsg.AddComponent("Ccy", pACcy);
    }
    // 6.汇钞标志
    CMsgComponent* pRCashExCd = oRmsg.GetComponent("CashExCode");
    if(pRCashExCd)
    {
        CMsgComponent* pACashExCd = new CMsgComponent(*pRCashExCd);
        oAmsg.AddComponent("CashExCode", pACashExCd);
    }
    // 7.证券方账户余额
    CBalance* pRScBal = dynamic_cast<CBalance*>(oRmsg.GetComponent("ScBal"));
    if(pRScBal)
    {
        CMsgComponent* pAScBal = new CMsgComponent(*pRScBal);
        oAmsg.AddComponent("ScBal", pAScBal);
    }
    // 8.摘要
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
    // 解析XML内容
    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_SUCCESS != doc.Parse(strXml.c_str()))
    {
        LOGE("XML解析不成功，XMLDocument.Parse");
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
    // 清除数据
    Clear();

    // 获取根节点属性值
    for (const tinyxml2::XMLAttribute* pa = pr->FirstAttribute(); pa; pa=pa->Next())
    {
        m_mapRootAttr[pa->Name()] = pa->Value();
    }

    // 获取消息类型
    tinyxml2::XMLElement* pMsgBody = pr->FirstChildElement()->FirstChildElement();
    m_strMsgType = pMsgBody->Name();
    if(!ParseComponents(pMsgBody))
    {
        LOGE("解析组件失败");
        return false;
    }

    return true;
}

bool CTxMsg::Parse(const string& strXml)
{
    // 解析数据
    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_SUCCESS != doc.Parse(strXml.c_str()))
    {
        LOGE("XML解析不成功，XMLDocument.Parse");
        return false;
    }

    // 获取根节点属性值
    tinyxml2::XMLElement* pr = doc.RootElement();
    return Parse(pr);
}

std::string CTxMsg::PackXml()
{
    // 包体
    string strBody;
    for (map<string, CMsgComponent*>::iterator i=m_mapComponent.begin(); i!=m_mapComponent.end(); ++i)
    {
        strBody += MMakeXmlElem(i->first, i->second->PackXml());
    }

    // 加包类型
    strBody = MMakeXmlElem(m_strMsgType, "\n" + strBody);
    strBody = MMakeXmlElem("MsgText", "\n" + strBody);

    // 计算校验值
    char buf[128] = {0};
    int nCheckSum = MCalCheckSum(strBody);
    sprintf(buf, "%d", nCheckSum);
    m_mapRootAttr["CheckSum"] = buf;
    // 计算包长
    sprintf(buf, "%05d", static_cast<int>(strBody.size()));
    m_mapRootAttr["Len"] = buf;

    // 包头属性长度
    size_t nAttrLen = 0;
    for (map<string, string>::iterator i=m_mapRootAttr.begin(); i!=m_mapRootAttr.end(); ++i)
    {
        nAttrLen += i->first.size() + i->second.size() + 4;
    }

    // 总包长度
    string pXmlSec[] = {"<IFTS", ">\n", "</IFTS>", ""};
    size_t nLen = strBody.size() + nAttrLen;
    for(int i=0; !pXmlSec[i].empty(); ++i)
    {
        nLen += pXmlSec[i].size();
    }
    sprintf(buf, "%05d", static_cast<int>(nLen));
    m_mapRootAttr["Len"] = buf;

    // 组包
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
    // 遍历消息体子元素
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
    // 拆分出类型中的序号
    string strType = m_strMsgType;
    size_t b = strType.find_first_of('.');
    size_t e = strType.find_last_of('.');
    if(b==string::npos || e==string::npos)
    {
        return "";
    }
    string strSeq = strType.substr(b+1, 3);
    int nSeq = atoi(strSeq.c_str());

    // 如果序号是偶数，则直接返回
    if((nSeq&1) == 0)
    {
        return strType;
    }

    // 生成应答序号类型
    char buf[32] = {0};
    sprintf(buf, "%03d", atoi(strSeq.c_str())+1);
    return strType.replace(b+1, 3, buf);
}

bool CTxMsg::IsReqMsg(const string& strXml)
{
    // 如果消息为应答包，则丢弃
    // 判断是否有返回组件信息:<Rst><Code></Code><Info></Info></Rst>
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

    // 检测顺序，为应答消息则直接丢掉
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
    // 打开文件
    ifstream oFin;
    oFin.open(strPath);
    if(!oFin.is_open())
    {
        return 0;
    }

    // 读取文件
    string strLine;
    string strXml;
    size_t pos = 0;
    while(getline(oFin, strLine))
    {
        // 查找头
        if(strXml.empty())
        {
            // 是否有头
            pos = strLine.find("<IFTS");
            if(pos == string::npos)
            {
                continue;
            }
            strXml = strLine.substr(pos);
            // 同行存在结尾标志
            pos = strXml.find("</IFTS>");
            if(pos != string::npos)
            {
                strXml = strXml.substr(0, pos+7);
                vecXml.push_back(strXml);
                strXml.clear();
            }
            continue;
        }
        // 查找尾巴
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
    // 内存清理
    for (map<string, CMsgComponent*>::iterator i=m_mapComponent.begin(); i!=m_mapComponent.end(); ++i)
    {
        delete i->second;
        i->second = NULL;
    }
    m_mapComponent.clear();
    m_mapRootAttr.clear();
    m_strMsgType = "";
}
