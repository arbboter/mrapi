#pragma once
#include "mrapi.h"
#include "tinyxml2.h"
#include <string>
#include <map>
#include <vector>
#include <Windows.h>
#include <time.h>

using namespace std;


// ��Ϣ����
#define MSG_TYPE_SESSION            "Sysm.001.01"        // �Ự��Ϣ
#define MSG_TYPE_SESSION_R          "Sysm.002.01"        // �Ự��ִ
#define MSG_TYPE_OPEN_ACCT          "Acmt.001.01"        // ����
#define MSG_TYPE_OPEN_ACCT_R        "Acmt.002.01"        // ������ִ
#define MSG_TYPE_CLOSE_ACCT         "Acmt.003.01"        // ������Ϣ
#define MSG_TYPE_CLOSE_ACCT_R       "Acmt.004.01"        // ������ִ
#define MSG_TYPE_MODIFY_ACCT        "Acmt.005.01"        // �˻��޸�
#define MSG_TYPE_MODIFY_ACCT_R      "Acmt.006.01"        // �˻��޸Ļ�ִ
#define MSG_TYPE_UPD_ACCT           "Acmt.007.01"        // �˻����
#define MSG_TYPE_UPD_ACCT_R         "Acmt.008.01"        // �˻������ִ
#define MSG_TYPE_QUERY_ACCT         "Acmt.009.01"        // �˻���ѯ
#define MSG_TYPE_QUERY_ACCT_R       "Acmt.010.01"        // �˻���ѯ��ִ
#define MSG_TYPE_TRADE              "Trf.001.01"         // ת��
#define MSG_TYPE_TRADE_R            "Trf.002.01"         // ת�˻�ִ
#define MSG_TYPE_UNTRADE            "Trf.003.01"         // ת�˳���
#define MSG_TYPE_UNTRADE_R          "Trf.004.01"         // ת�˳�����ִ
#define MSG_TYPE_QRY_TRD_RST        "Trf.005.01"         // ���׽����ѯ
#define MSG_TYPE_QRY_TRD_RST_R      "Trf.006.01"         // ���׽����ѯ��ִ
#define MSG_TYPE_SETT_WELL          "Trf.007.01"         // ��Ϣ
#define MSG_TYPE_SETT_WELL_R        "Trf.008.01"         // ��Ϣ��ִ
#define MSG_TYPE_CHECK_SEQ          "Stmt.001.01"        // ����
#define MSG_TYPE_CHECK_SEQ_R        "Stmt.002.01"        // ���˻�ִ
#define MSG_TYPE_SETTFILE_OK        "Stmt.003.01"        // �������ݾ���
#define MSG_TYPE_SETTFILE_OK_R      "Stmt.004.01"        // �������ݾ�����ִ
#define MSG_TYPE_FILE_OPER          "File.001.01"        // �ļ�����
#define MSG_TYPE_FILE_OPER_R        "File.002.01"        // �ļ�������ִ

// ��������
// ��ȡ��ʱʱ����Ϣ
std::string MGetCurDate(const char *format);
// ��ȡ��ˮ��
std::string MGetLsh();
// �������п���
std::string MMakeBkAcct(const string& strScAcct, const string& strBkAcct);
// ��װXML��ǩ
std::string MMakeXmlElem(const string& strTag, const string& strText);
// ����У���
int MCalCheckSum(const string& strBody);
// ��ȡԪ�ض���
const tinyxml2::XMLElement* XmlFindFirst(const tinyxml2::XMLElement* pe, const string& strName);
// ��ȡԪ������
std::string XmlTextFirst(const tinyxml2::XMLElement* pe, const string& strName);

// ��Ϣ���
class CMsgComponent
{
public:
    // �洢XML����������ֵ�ֵ��KEYΪ��ǩֵ��VALΪ��ǩ����
    map<string, string>    _map_val;

    // �������XML����-�ַ���
    virtual bool Parse(const string& strXml);
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe);
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CMsgComponent(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;
        for (map<string, string>::iterator i=_map_val.begin(); i!=_map_val.end(); ++i)
        {
            xml += "<" + i->first + ">" + i->second + "</" + i->first + ">\n";
        }
        return xml;
    }
};

// ͨѶ��Ϣ����
class CTxMsg
{
public:
    // ҵ��Ҫ�����
    map<string, CMsgComponent*> m_mapComponent;
    // ���ڵ�����
    map<string, string>         m_mapRootAttr;
    // ��Ϣ���ͱ�ǩ
    string                      m_strMsgType;

public:
    // ���캯��
    CTxMsg(){}
    // �������캯��
    CTxMsg(const CTxMsg& m)
    {
        m.CopyTo(this);
    }
    // ���ؿ������캯��
    CTxMsg & operator= (const CTxMsg& m)
    {
        m.CopyTo(this);
        return *this;
    }
    // ��¡������
    void CopyTo(CTxMsg* p) const
    {
        if(!p) return;
        // ��ͨ��������
        p->m_strMsgType = m_strMsgType;
        p->m_mapRootAttr = m_mapRootAttr;
        // ���
        for (map<string, CMsgComponent*>::const_iterator i=m_mapComponent.begin(); i!=m_mapComponent.end(); ++i)
        {
            if(i->second)
            {
                p->m_mapComponent[i->first] = i->second->Clone();
            }
            else
            {
                p->m_mapComponent[i->first] = NULL;
            }
        }
    }

public:
    // ��������
    ~CTxMsg();
    // ��Ϣ����
    bool Parse(const string& strXml);
    // ��Ϣ���
    string PackXml();

    // ��Ϣ����
    bool Parse(tinyxml2::XMLElement* pr);
    // ������Ϣ���
    bool ParseComponents(tinyxml2::XMLElement* pb);
    // ��ȡ���
    CMsgComponent* GetComponent(const string& strName);
    // ������
    void AddComponent(const string& strName, CMsgComponent* p);
    // ��ȡӦ�������
    string GetRetMsgType() const;
    // �ж��Ƿ�Ϊ������Ϣ
    static bool IsReqMsg(const string& strXml);
    // ���ļ���ȡ��Ϣ
    static int LoadMsg(const string& strPath, vector<string>& vecXml);

public:
    // �������
    void Clear();
};

#define MSG_SRC_SEND     100
#define MSG_SRC_FILE     101

// ������Ϣ����
class CMsgInfo
{
public:
    // ��Ϣ����
    const char*     psPkg;
    // ��Ϣ����
    int             iPkgLen;
    // ��Ϣ����
    CTxMsg          oPkg;
    // ��Ϣ����
    STUMsgProperty  oMsgPropery;
    // ��ʱʱ��
    int             iMillSecTimeo;
    // ��Ϣ��Դ
    int             nMsgSrc;
};

// �Զ�������ǰ������
class CMessgeHeader;
class CReturnResult;
class CReference;
class CInstitution;
class CCustomer;
class CAgent;
class CAccount;
class CPassword;
class CBalance;
class CFileInfo;
class CAccountStatusStatement;
class CAccountStatusStatementConfirm;
class CAccountTradeStatement;
class CAccountTradeStatementConfirm;
class CTransferStatement;
class CTransferStatementConfirm;
class CBalanceStatement;
class CBalanceStatementConfirm;

// 8.2�����ؽ����ReturnResult��
class CReturnResult: public CMsgComponent
{
public:
    string                      strCode;                         // ������ <Code>	[1..1]	ReturnCode
    string                      strInfo;                         // ������Ϣ <Info>	[0..1]	Max128Text


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CReturnResult(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Code>" + strCode + "</Code>\n";
        xml += "<Info>" + strInfo + "</Info>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strCode = XmlTextFirst(pe, "Code");
        strInfo = XmlTextFirst(pe, "Info");
        return true;
    }
};

// 8.3����ˮ��(Reference)
class CReference: public CMsgComponent
{
public:
    string                      strReference;                    // ��ˮ�� <Ref>	[1..1]	Max35Text
    string                      strRefrenceIssureType;           // ��ˮ�ŷ��������� <IssrType>	[1..1]	InstitutionType
    string                      strReferenceIssure;              // ������ <RefIssr>	[0..1]	Max35Text


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CReference(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Ref>" + strReference + "</Ref>\n";
        xml += "<IssrType>" + strRefrenceIssureType + "</IssrType>\n";
        xml += "<RefIssr>" + strReferenceIssure + "</RefIssr>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strReference = XmlTextFirst(pe, "Ref");
        strRefrenceIssureType = XmlTextFirst(pe, "IssrType");
        strReferenceIssure = XmlTextFirst(pe, "RefIssr");
        return true;
    }
};

// 8.4��������Ϣ��Institution��
class CInstitution: public CMsgComponent
{
public:
    string                      strInstitutionType;              // �������� <InstType>	[1..1]	InstitutionType
    string                      strInstitutionIdentifier;        // ������ʶ <InstId>	[1..1]	Max35Text
    string                      strInstitutionName;              // �������� <InstNm>	[0..1]	Max70Text
    string                      strBranchIdentifier;             // ��֧�������� <BrchId>	[0..1]	Max35Text
    string                      strBranchName;                   // ��֧�������� <BrchNm>	[0..1]	Max70Text
    string                      strSubBranchIdentifier;          // ����� <SubBrchId>	[0..1]	Max35Text
    string                      strSubBranchName;                // �������� <SubBrchNm>	[0..1]	Max70Text


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CInstitution(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<InstType>" + strInstitutionType + "</InstType>\n";
        xml += "<InstId>" + strInstitutionIdentifier + "</InstId>\n";
        xml += "<InstNm>" + strInstitutionName + "</InstNm>\n";
        xml += "<BrchId>" + strBranchIdentifier + "</BrchId>\n";
        xml += "<BrchNm>" + strBranchName + "</BrchNm>\n";
        xml += "<SubBrchId>" + strSubBranchIdentifier + "</SubBrchId>\n";
        xml += "<SubBrchNm>" + strSubBranchName + "</SubBrchNm>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strInstitutionType = XmlTextFirst(pe, "InstType");
        strInstitutionIdentifier = XmlTextFirst(pe, "InstId");
        strInstitutionName = XmlTextFirst(pe, "InstNm");
        strBranchIdentifier = XmlTextFirst(pe, "BrchId");
        strBranchName = XmlTextFirst(pe, "BrchNm");
        strSubBranchIdentifier = XmlTextFirst(pe, "SubBrchId");
        strSubBranchName = XmlTextFirst(pe, "SubBrchNm");
        return true;
    }
};

// 8.5���ͻ���Ϣ��Customer��
class CCustomer: public CMsgComponent
{
public:
    string                      strName;                         // �ͻ����� <Name>	[1..1]	Max70Text
    string                      strCertificationType;            // ֤������ <CertType>	[1..1]	CertificationType
    string                      strCertificationIdentifier;      // ֤������ <CertId>	[1..1]	Max35Text
    string                      strType;                         // �ͻ����� <Type>	[0..1]	CustomerType
    string                      strGender;                       // �ͻ��Ա� <Gender>	[0..1]	GenderCode
    string                      strNationality;                  // �ͻ����� <Ntnl>	[0..1]	CountryCode
    string                      strAddress;                      // ͨ�ŵ�ַ <Addr>	[0..1]	Max70Text
    string                      strPostcode;                     // �������� <PstCd>	[0..1]	Max35Text
    string                      strEmail;                        // �����ʼ� <Email>	[0..1]	Max70Text
    string                      strFax;                          // ���� <Fax>	[0..1]	Max35Text
    string                      strMobile;                       // �ֻ� <Mobile>	[0..1]	Max35Text
    string                      strTelephone;                    // �绰 <Tel>	[0..1]	Max35Text
    string                      strBp;                           // ���� <Bp>	[0..1]	Max35Text


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CCustomer(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Name>" + strName + "</Name>\n";
        xml += "<CertType>" + strCertificationType + "</CertType>\n";
        xml += "<CertId>" + strCertificationIdentifier + "</CertId>\n";
        xml += "<Type>" + strType + "</Type>\n";
        xml += "<Gender>" + strGender + "</Gender>\n";
        xml += "<Ntnl>" + strNationality + "</Ntnl>\n";
        xml += "<Addr>" + strAddress + "</Addr>\n";
        xml += "<PstCd>" + strPostcode + "</PstCd>\n";
        xml += "<Email>" + strEmail + "</Email>\n";
        xml += "<Fax>" + strFax + "</Fax>\n";
        xml += "<Mobile>" + strMobile + "</Mobile>\n";
        xml += "<Tel>" + strTelephone + "</Tel>\n";
        xml += "<Bp>" + strBp + "</Bp>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strName = XmlTextFirst(pe, "Name");
        strCertificationType = XmlTextFirst(pe, "CertType");
        strCertificationIdentifier = XmlTextFirst(pe, "CertId");
        strType = XmlTextFirst(pe, "Type");
        strGender = XmlTextFirst(pe, "Gender");
        strNationality = XmlTextFirst(pe, "Ntnl");
        strAddress = XmlTextFirst(pe, "Addr");
        strPostcode = XmlTextFirst(pe, "PstCd");
        strEmail = XmlTextFirst(pe, "Email");
        strFax = XmlTextFirst(pe, "Fax");
        strMobile = XmlTextFirst(pe, "Mobile");
        strTelephone = XmlTextFirst(pe, "Tel");
        strBp = XmlTextFirst(pe, "Bp");
        return true;
    }
};

// 8.6����������Ϣ��Agent��
class CAgent: public CMsgComponent
{
public:
    string                      strName;                         // ���������� <Name>	[0..1]	Max70Text
    string                      strCertificationType;            // ֤������ <CertType>	[0..1]	CertificationType
    string                      strCertificationIdentifier;      // ֤������ <CertId>	[0..1]	Max35Text
    string                      strAuthentication;               // ������Ȩ�� <Auth>	[0..1]	AgentAuthCode
    string                      strGender;                       // �������Ա� <Gender>	[0..1]	GenderCode
    string                      strNationality;                  // �����˹��� <Ntnl>	[0..1]	CountryCode
    string                      strAddress;                      // ͨ�ŵ�ַ <Addr>	[0..1]	Max70Text
    string                      strPostcode;                     // �������� <PstCd>	[0..1]	Max35Text
    string                      strEmail;                        // �����ʼ� <Email>	[0..1]	Max70Text
    string                      strFax;                          // ���� <Fax>	[0..1]	Max35Text
    string                      strMobile;                       // �ֻ� <Mobile>	[0..1]	Max35Text
    string                      strTelephone;                    // �绰 <Tel>	[0..1]	Max35Text
    string                      strBp;                           // ���� <Bp>	[0..1]	Max35Text
    string                      strBeginDate;                    // ����ʼ�� <BgnDt>	[0..1]	Date
    string                      strEndDate;                      // �������� <EndDt>	[0..1]	Date


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CAgent(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Name>" + strName + "</Name>\n";
        xml += "<CertType>" + strCertificationType + "</CertType>\n";
        xml += "<CertId>" + strCertificationIdentifier + "</CertId>\n";
        xml += "<Auth>" + strAuthentication + "</Auth>\n";
        xml += "<Gender>" + strGender + "</Gender>\n";
        xml += "<Ntnl>" + strNationality + "</Ntnl>\n";
        xml += "<Addr>" + strAddress + "</Addr>\n";
        xml += "<PstCd>" + strPostcode + "</PstCd>\n";
        xml += "<Email>" + strEmail + "</Email>\n";
        xml += "<Fax>" + strFax + "</Fax>\n";
        xml += "<Mobile>" + strMobile + "</Mobile>\n";
        xml += "<Tel>" + strTelephone + "</Tel>\n";
        xml += "<Bp>" + strBp + "</Bp>\n";
        xml += "<BgnDt>" + strBeginDate + "</BgnDt>\n";
        xml += "<EndDt>" + strEndDate + "</EndDt>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strName = XmlTextFirst(pe, "Name");
        strCertificationType = XmlTextFirst(pe, "CertType");
        strCertificationIdentifier = XmlTextFirst(pe, "CertId");
        strAuthentication = XmlTextFirst(pe, "Auth");
        strGender = XmlTextFirst(pe, "Gender");
        strNationality = XmlTextFirst(pe, "Ntnl");
        strAddress = XmlTextFirst(pe, "Addr");
        strPostcode = XmlTextFirst(pe, "PstCd");
        strEmail = XmlTextFirst(pe, "Email");
        strFax = XmlTextFirst(pe, "Fax");
        strMobile = XmlTextFirst(pe, "Mobile");
        strTelephone = XmlTextFirst(pe, "Tel");
        strBp = XmlTextFirst(pe, "Bp");
        strBeginDate = XmlTextFirst(pe, "BgnDt");
        strEndDate = XmlTextFirst(pe, "EndDt");
        return true;
    }
};

// 8.8�����루Password��
class CPassword: public CMsgComponent
{
public:
    string                      strType;                         // �������� <Type>	[0..1]	PasswordType
    string                      strEncryMode;                    // ���ܷ�ʽ <Enc>	[0..1]	EncryMode
    string                      strPassword;                     // ���� <Pwd>	[0..1]	Max35Text


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CPassword(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Type>" + strType + "</Type>\n";
        xml += "<Enc>" + strEncryMode + "</Enc>\n";
        xml += "<Pwd>" + strPassword + "</Pwd>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strType = XmlTextFirst(pe, "Type");
        strEncryMode = XmlTextFirst(pe, "Enc");
        strPassword = XmlTextFirst(pe, "Pwd");
        return true;
    }
};

// 8.9�����(Balance)
class CBalance: public CMsgComponent
{
public:
    string                      strBalanceType;                  // ������� <Type>	[0..1]	BalanceType
    string                      strBalance;                      // ��� <Bal>	[1..1]	Amount


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CBalance(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Type>" + strBalanceType + "</Type>\n";
        xml += "<Bal>" + strBalance + "</Bal>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strBalanceType = XmlTextFirst(pe, "Type");
        strBalance = XmlTextFirst(pe, "Bal");
        return true;
    }
};

// 8.10���ļ���Ϣ(FileInfo)
class CFileInfo: public CMsgComponent
{
public:
    string                      strFileBusCode;                  // �ļ�ҵ���� <BusCode>	[0..1]	FileBusinessCode
    string                      strBusinessDate;                 // �ļ�ҵ������ <BusDate>	[0..1]	Date
    string                      strHost;                         // �ļ�������� <Host>	[0..1]	Max35Text
    string                      strFileName;                     // �ļ����� <FileName>	[1..1]	Max128Text
    string                      strFileLength;                   // �ļ����� <FileLen>	[0..1]	Number
    string                      strFileTime;                     // �ļ�ʱ�� <FileTime>	[0..1]	DateTime
    string                      strFileMac;                      // �ļ�У���� <FileMac>	[0..1]	Max128Text


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CFileInfo(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<BusCode>" + strFileBusCode + "</BusCode>\n";
        xml += "<BusDate>" + strBusinessDate + "</BusDate>\n";
        xml += "<Host>" + strHost + "</Host>\n";
        xml += "<FileName>" + strFileName + "</FileName>\n";
        xml += "<FileLen>" + strFileLength + "</FileLen>\n";
        xml += "<FileTime>" + strFileTime + "</FileTime>\n";
        xml += "<FileMac>" + strFileMac + "</FileMac>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strFileBusCode = XmlTextFirst(pe, "BusCode");
        strBusinessDate = XmlTextFirst(pe, "BusDate");
        strHost = XmlTextFirst(pe, "Host");
        strFileName = XmlTextFirst(pe, "FileName");
        strFileLength = XmlTextFirst(pe, "FileLen");
        strFileTime = XmlTextFirst(pe, "FileTime");
        strFileMac = XmlTextFirst(pe, "FileMac");
    }
};

// 8.1����Ϣͷ��MessgeHeader��
class CMessgeHeader: public CMsgComponent
{
public:
    string                      strVersion;                      // �汾 <Ver>	[1..1]	Max35Text
    string                      strSystemType;                   // Ӧ��ϵͳ���� <SysType>	[1..1]	SystemType
    string                      strInstructionCode;              // ҵ������ <InstrCd>	[1..1]	InstructionCode
    string                      strTradeSource;                  // ���׷��� <TradSrc>	[1..1	InstitutionType
    CInstitution                oSender;                         // ���ͻ��� <Sender>	[1..1]	Institution
    CInstitution                oRecver;                         // ���ջ��� <Recver>	[1..1]	Institution
    string                      strCreateDate;                   // �������� <Date>	[0..1]	Date
    string                      strCreateTime;                   // ����ʱ�� <Time>	[0..1]	Time
    CReference                  oRefrence;                       // ��Ϣ��ˮ�� <Ref>	[0..1]	Reference
    CReference                  oRelatedReference;               // �����Ϣ��ˮ�� <RltdRef>	[0..1]	Reference
    string                      strLastFragment;                 // ����Ƭ��־ <LstFrag>	[0..1]	YesNoIndicator


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CMessgeHeader(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Ver>" + strVersion + "</Ver>\n";
        xml += "<SysType>" + strSystemType + "</SysType>\n";
        xml += "<InstrCd>" + strInstructionCode + "</InstrCd>\n";
        xml += "<TradSrc>" + strTradeSource + "</TradSrc>\n";
        xml += "<Sender>" + oSender.PackXml() + "</Sender>\n";
        xml += "<Recver>" + oRecver.PackXml() + "</Recver>\n";
        xml += "<Date>" + strCreateDate + "</Date>\n";
        xml += "<Time>" + strCreateTime + "</Time>\n";
        xml += "<Ref>" + oRefrence.PackXml() + "</Ref>\n";
        xml += "<RltdRef>" + oRelatedReference.PackXml() + "</RltdRef>\n";
        xml += "<LstFrag>" + strLastFragment + "</LstFrag>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strVersion = XmlTextFirst(pe, "Ver");
        strSystemType = XmlTextFirst(pe, "SysType");
        strInstructionCode = XmlTextFirst(pe, "InstrCd");
        strTradeSource = XmlTextFirst(pe, "TradSrc");
        oSender.Parse(XmlFindFirst(pe, "Sender"));
        oRecver.Parse(XmlFindFirst(pe, "Recver"));
        strCreateDate = XmlTextFirst(pe, "Date");
        strCreateTime = XmlTextFirst(pe, "Time");
        oRefrence.Parse(XmlFindFirst(pe, "Ref"));
        oRelatedReference.Parse(XmlFindFirst(pe, "RltdRef"));
        strLastFragment = XmlTextFirst(pe, "LstFrag");
        return true;
    }
};

// 8.7���˻�<Account>
class CAccount: public CMsgComponent
{
public:
    string                      strAccountIdentification;        // �˺� <Id>	[1..1]	Max35Text
    string                      strAccountName;                  // �������� <Name>	[0..1]	Max70Text
    string                      strAccountType;                  // �˻���� <Type>	[0..1]	AccountType
    string                      strAccountStatus;                // �˻�״̬ <Status>	[0..1]	AccountStatus
    CPassword                   oPassword;                       // ���� <Pwd>	[0..n]	Password
    string                      strRegisterDate;                 // �������� <RegDt>	[0..1]	Date
    string                      strValidDate;                    // ��Ч���� <VldDt>	[0..1]	Date
    CInstitution                oAccountService;                 // �˻������� <AcctSvcr>	[0..1]	Institution


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CAccount(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Id>" + strAccountIdentification + "</Id>\n";
        xml += "<Name>" + strAccountName + "</Name>\n";
        xml += "<Type>" + strAccountType + "</Type>\n";
        xml += "<Status>" + strAccountStatus + "</Status>\n";
        xml += "<Pwd>" + oPassword.PackXml() + "</Pwd>\n";
        xml += "<RegDt>" + strRegisterDate + "</RegDt>\n";
        xml += "<VldDt>" + strValidDate + "</VldDt>\n";
        xml += "<AcctSvcr>" + oAccountService.PackXml() + "</AcctSvcr>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strAccountIdentification = XmlTextFirst(pe, "Id");
        strAccountName = XmlTextFirst(pe, "Name");
        strAccountType = XmlTextFirst(pe, "Type");
        strAccountStatus = XmlTextFirst(pe, "Status");
        oPassword.Parse(XmlFindFirst(pe, "Pwd"));
        strRegisterDate = XmlTextFirst(pe, "RegDt");
        strValidDate = XmlTextFirst(pe, "VldDt");
        oAccountService.Parse(XmlFindFirst(pe, "AcctSvcr"));
        return true;
    }
};

// 8.11���˻�״̬��ϸ��AccountStatusStatement��
class CAccountStatusStatement: public CMsgComponent
{
public:
    CCustomer                   oCustomer;                       // �ͻ���Ϣ <Cust>	[1..1]	Customer
    CAccount                    oBankAccount;                    // �����˺� <BkAcct>	[0..1]	Account
    CAccount                    oSecuritiesAccount;              // ֤ȯ���˻� <ScAcct>	[1..1]	Account
    string                      strManageStatus;                 // ���״̬ <MngSt>	[1..1]	ManagerStatus
    string                      strCurrency;                     // ���� <Ccy>	[0..1]	CurrencyCode
    string                      strCashExCode;                   // �㳮��־ <CashExCd>	[0..1]	CashExCode
    string                      strDate;                         // �������� <Date>	[0..1]	Date


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CAccountStatusStatement(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Cust>" + oCustomer.PackXml() + "</Cust>\n";
        xml += "<BkAcct>" + oBankAccount.PackXml() + "</BkAcct>\n";
        xml += "<ScAcct>" + oSecuritiesAccount.PackXml() + "</ScAcct>\n";
        xml += "<MngSt>" + strManageStatus + "</MngSt>\n";
        xml += "<Ccy>" + strCurrency + "</Ccy>\n";
        xml += "<CashExCd>" + strCashExCode + "</CashExCd>\n";
        xml += "<Date>" + strDate + "</Date>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        oCustomer.Parse(XmlFindFirst(pe, "Cust"));
        oBankAccount.Parse(XmlFindFirst(pe, "BkAcct"));
        oSecuritiesAccount.Parse(XmlFindFirst(pe, "ScAcct"));
        strManageStatus = XmlTextFirst(pe, "MngSt");
        strCurrency = XmlTextFirst(pe, "Ccy");
        strCashExCode = XmlTextFirst(pe, "CashExCd");
        strDate = XmlTextFirst(pe, "Date");
        return true;
    }
};

// 8.12���˻�״̬���˽����AccountStatusStatementConfirm��
class CAccountStatusStatementConfirm: public CMsgComponent
{
public:
    CAccountStatusStatement     oBankEntry;                      // ���з��˻�״̬��ϸ��¼ <BkEntry>	[1..1]	AccountStatusStatement
    string                      strSecuritiesEntry;              // ֤ȯ���˻�״̬��ϸ��¼ <ScEntry>	[1..1]	AccountStatusStatementt
    CReturnResult               oCheckResult;                    // ���˽�� <ChkRst>	[1..1]	ReturnResult


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CAccountStatusStatementConfirm(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<BkEntry>" + oBankEntry.PackXml() + "</BkEntry>\n";
        xml += "<ScEntry>" + strSecuritiesEntry + "</ScEntry>\n";
        xml += "<ChkRst>" + oCheckResult.PackXml() + "</ChkRst>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        oBankEntry.Parse(XmlFindFirst(pe, "BkEntry"));
        strSecuritiesEntry = XmlTextFirst(pe, "ScEntry");
        oCheckResult.Parse(XmlFindFirst(pe, "ChkRst"));
        return true;
    }
};

// 8.13���˻�������ϸ��AccountTradeStatement��
class CAccountTradeStatement: public CMsgComponent
{
public:
    CReference                  oReference;                      // ��ˮ�� <Ref>	[0..2]	Reference
    string                      strTradeSource;                  // ���׷��� <TradSrc>	[0..1]	InstitutionType
    string                      strInstructionCode;              // ҵ������ <InstrCd>	[0..1]	InstructionCode
    CCustomer                   oCustomer;                       // �ͻ���Ϣ <Cust>	[0..1]	Customer
    CAccount                    oBankAccount;                    // ���з��˻� <BkAcct>	[0..1]	Account
    CAccount                    oSecuritiesAccount;              // ֤ȯ���˻� <ScAcct>	[0..1]	Account
    string                      strCurrency;                     // ���� <Ccy>	[0..1]	CurrencyCode
    string                      strCashExCode;                   // �㳮��־ <CashExCd>	[0..1]	CashExCode
    CBalance                    oSecuritiesBalance;              // ֤ȯ���˻���� <ScBal>	[0..n]	Balance
    string                      strDate;                         // �������� <Date>	[0..1]	Date
    string                      strTime;                         // ����ʱ�� <Time>	[0..1]	Time
    string                      strDigest;                       // ҵ��ժҪ <Dgst>	[0..1]	Max35Text


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CAccountTradeStatement(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Ref>" + oReference.PackXml() + "</Ref>\n";
        xml += "<TradSrc>" + strTradeSource + "</TradSrc>\n";
        xml += "<InstrCd>" + strInstructionCode + "</InstrCd>\n";
        xml += "<Cust>" + oCustomer.PackXml() + "</Cust>\n";
        xml += "<BkAcct>" + oBankAccount.PackXml() + "</BkAcct>\n";
        xml += "<ScAcct>" + oSecuritiesAccount.PackXml() + "</ScAcct>\n";
        xml += "<Ccy>" + strCurrency + "</Ccy>\n";
        xml += "<CashExCd>" + strCashExCode + "</CashExCd>\n";
        xml += "<ScBal>" + oSecuritiesBalance.PackXml() + "</ScBal>\n";
        xml += "<Date>" + strDate + "</Date>\n";
        xml += "<Time>" + strTime + "</Time>\n";
        xml += "<Dgst>" + strDigest + "</Dgst>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        oReference.Parse(XmlFindFirst(pe, "Ref"));
        strTradeSource = XmlTextFirst(pe, "TradSrc");
        strInstructionCode = XmlTextFirst(pe, "InstrCd");
        oCustomer.Parse(XmlFindFirst(pe, "Cust"));
        oBankAccount.Parse(XmlFindFirst(pe, "BkAcct"));
        oSecuritiesAccount.Parse(XmlFindFirst(pe, "ScAcct"));
        strCurrency = XmlTextFirst(pe, "Ccy");
        strCashExCode = XmlTextFirst(pe, "CashExCd");
        oSecuritiesBalance.Parse(XmlFindFirst(pe, "ScBal"));
        strDate = XmlTextFirst(pe, "Date");
        strTime = XmlTextFirst(pe, "Time");
        strDigest = XmlTextFirst(pe, "Dgst");
        return true;
    }
};

// 8.14���˻����׶��˽����AccountTradeStatementConfirm��
class CAccountTradeStatementConfirm: public CMsgComponent
{
public:
    CAccountTradeStatement      oBankEntry;                      // ���з��˻�������ϸ <BkEntry>	[1..1]	AccountTradeStatement
    CAccountTradeStatement      oSecuritiesEntry;                // ���з��˻�������ϸ <ScEntry>	[1..1]	AccountTradeStatement
    CReturnResult               oCheckResult;                    // ���˽�� <ChkRst>	[1..1]	ReturnResult


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CAccountTradeStatementConfirm(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<BkEntry>" + oBankEntry.PackXml() + "</BkEntry>\n";
        xml += "<ScEntry>" + oSecuritiesEntry.PackXml() + "</ScEntry>\n";
        xml += "<ChkRst>" + oCheckResult.PackXml() + "</ChkRst>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        oBankEntry.Parse(XmlFindFirst(pe, "BkEntry"));
        oSecuritiesEntry.Parse(XmlFindFirst(pe, "ScEntry"));
        oCheckResult.Parse(XmlFindFirst(pe, "ChkRst"));
        return true;
    }
};

// 8.15��ת����ϸ(TransferStatement)
class CTransferStatement: public CMsgComponent
{
public:
    CReference                  oReference;                      // ��ˮ�� <Ref>	[1..2]	Reference
    string                      strTradeSource;                  // ���׷��� <TradSrc>	[1..1]	InstitutionType
    string                      strInstructionCode;              // ҵ������ <InstrCd>	[1..1]	InstructionCode
    CCustomer                   oCustomer;                       // �ͻ���Ϣ <Cust>	[0..1]	Customer
    CAccount                    oBankAccount;                    // ���з��˻� <BkAcct>	[1..1]	Account
    CAccount                    oSecuritiesAccount;              // ֤ȯ���˻� <ScAcct>	[1..1]	Account
    string                      strCashExCode;                   // �㳮��־ <CashExCd>	[0..1]	CashExCode
    string                      strCurrency;                     // ���� <Ccy>	[0..1]	CurrencyCode
    string                      strAmount;                       // ������� <Amt>	[1..1]	Amount
    string                      strDate;                         // �������� <Date>	[0..1]	Date
    string                      strTime;                         // ����ʱ�� <Time>	[0..1]	Time
    string                      strDigest;                       // ҵ��ժҪ <Dgst>	[0..1]	Max35Text


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CTransferStatement(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Ref>" + oReference.PackXml() + "</Ref>\n";
        xml += "<TradSrc>" + strTradeSource + "</TradSrc>\n";
        xml += "<InstrCd>" + strInstructionCode + "</InstrCd>\n";
        xml += "<Cust>" + oCustomer.PackXml() + "</Cust>\n";
        xml += "<BkAcct>" + oBankAccount.PackXml() + "</BkAcct>\n";
        xml += "<ScAcct>" + oSecuritiesAccount.PackXml() + "</ScAcct>\n";
        xml += "<CashExCd>" + strCashExCode + "</CashExCd>\n";
        xml += "<Ccy>" + strCurrency + "</Ccy>\n";
        xml += "<Amt>" + strAmount + "</Amt>\n";
        xml += "<Date>" + strDate + "</Date>\n";
        xml += "<Time>" + strTime + "</Time>\n";
        xml += "<Dgst>" + strDigest + "</Dgst>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        oReference.Parse(XmlFindFirst(pe, "Ref"));
        strTradeSource = XmlTextFirst(pe, "TradSrc");
        strInstructionCode = XmlTextFirst(pe, "InstrCd");
        oCustomer.Parse(XmlFindFirst(pe, "Cust"));
        oBankAccount.Parse(XmlFindFirst(pe, "BkAcct"));
        oSecuritiesAccount.Parse(XmlFindFirst(pe, "ScAcct"));
        strCashExCode = XmlTextFirst(pe, "CashExCd");
        strCurrency = XmlTextFirst(pe, "Ccy");
        strAmount = XmlTextFirst(pe, "Amt");
        strDate = XmlTextFirst(pe, "Date");
        strTime = XmlTextFirst(pe, "Time");
        strDigest = XmlTextFirst(pe, "Dgst");
        return true;
    }
};

// 8.16��ת�˶��˽��(TransferStatementConfirm)
class CTransferStatementConfirm: public CMsgComponent
{
public:
    CTransferStatement          oBankEntry;                      // ���з�ת����ϸ <BkEntry>	[0..1]	TransferStatement
    CTransferStatement          oSecuritiesEntry;                // ֤ȯ��ת����ϸ <ScEntry>	[0..1]	TransferStatement
    CReturnResult               oCheckResult;                    // ���˽�� <ChkRst>	[1..1]	ReturnResult


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CTransferStatementConfirm(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<BkEntry>" + oBankEntry.PackXml() + "</BkEntry>\n";
        xml += "<ScEntry>" + oSecuritiesEntry.PackXml() + "</ScEntry>\n";
        xml += "<ChkRst>" + oCheckResult.PackXml() + "</ChkRst>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        oBankEntry.Parse(XmlFindFirst(pe, "BkEntry"));
        oSecuritiesEntry.Parse(XmlFindFirst(pe, "ScEntry"));
        oCheckResult.Parse(XmlFindFirst(pe, "ChkRst"));
        return true;
    }
};

// 8.17���˻������ϸ(BalanceStatement)
class CBalanceStatement: public CMsgComponent
{
public:
    CCustomer                   oCustomer;                       // �ͻ���Ϣ <Cust>	[0..1]	Customer
    CAccount                    oBankAccount;                    // ���з��˻� <BkAcct>	[0..1]	Account
    CAccount                    oSecuritiesAccount;              // ֤ȯ���˻� <ScAcct>	[0..1]	Account
    string                      strCashExCode;                   // �㳮��־ <CashExCd>	[0..1]	CashExCode
    string                      strCurrency;                     // ���� <Ccy>	[0..1]	CurrencyCode
    CBalance                    oSecuritiesBalance;              // �ʽ��˻���� <ScBal>	[0..n]	Balance
    string                      strDate;                         // �������� <Date>	[0..1]	Date


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CBalanceStatement(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<Cust>" + oCustomer.PackXml() + "</Cust>\n";
        xml += "<BkAcct>" + oBankAccount.PackXml() + "</BkAcct>\n";
        xml += "<ScAcct>" + oSecuritiesAccount.PackXml() + "</ScAcct>\n";
        xml += "<CashExCd>" + strCashExCode + "</CashExCd>\n";
        xml += "<Ccy>" + strCurrency + "</Ccy>\n";
        xml += "<ScBal>" + oSecuritiesBalance.PackXml() + "</ScBal>\n";
        xml += "<Date>" + strDate + "</Date>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        oCustomer.Parse(XmlFindFirst(pe, "Cust"));
        oBankAccount.Parse(XmlFindFirst(pe, "BkAcct"));
        oSecuritiesAccount.Parse(XmlFindFirst(pe, "ScAcct"));
        strCashExCode = XmlTextFirst(pe, "CashExCd");
        strCurrency = XmlTextFirst(pe, "Ccy");
        oSecuritiesBalance.Parse(XmlFindFirst(pe, "ScBal"));
        strDate = XmlTextFirst(pe, "Date");
        return true;
    }
};

// 8.18���˻������˽��(BalanceStatementConfirm)
class CBalanceStatementConfirm: public CMsgComponent
{
public:
    CBalanceStatement           oBankEntry;                      // ���з��˻������ϸ <BkEntry>	[1..1]	BalanceStatement
    CBalanceStatement           oSecuritiesEntry;                // ֤ȯ���˻������ϸ <ScEntry>	[1..1]	BalanceStatement
    CReturnResult               oCheckResult;                    // ���˽�� <ChkRst>	[0..1]	ReturnResult


public:
    // �������
    virtual CMsgComponent* Clone()
    {
        return new CBalanceStatementConfirm(*this);
    }
    // �������ΪXML����
    virtual string PackXml()
    {
        string xml;

        xml += "<BkEntry>" + oBankEntry.PackXml() + "</BkEntry>\n";
        xml += "<ScEntry>" + oSecuritiesEntry.PackXml() + "</ScEntry>\n";
        xml += "<ChkRst>" + oCheckResult.PackXml() + "</ChkRst>\n";

        return xml;
    }
    // tinyxml2Ԫ�ض���
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        oBankEntry.Parse(XmlFindFirst(pe, "BkEntry"));
        oSecuritiesEntry.Parse(XmlFindFirst(pe, "ScEntry"));
        oCheckResult.Parse(XmlFindFirst(pe, "ChkRst"));
        return true;
    }
};


class CIFTSResp
{
public:
    CIFTSResp(void);
    ~CIFTSResp(void);

    bool MakeRespPkg(CMsgInfo& oReq, CMsgInfo& oResp);

public:
    string      m_strRetCode;   // ���ش���
    string      m_strRetInfo;   // ������Ϣ
    string      m_BankReqFile;  // ������������
    string      m_BankAnsFile;  // ����Ӧ������

private:
    void LoadConfig();

private:
    bool MakeRespPkgSession(CMsgInfo& oReq, CMsgInfo& oResp);
    bool MakeRespPkgTrade(CMsgInfo& oReq, CMsgInfo& oResp);
    bool MakeRespPkgOpenAcct(CMsgInfo& oReq, CMsgInfo& oResp);
};