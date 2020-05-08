#pragma once
#include "mrapi.h"
#include "tinyxml2.h"
#include <string>
#include <map>
#include <vector>
#include <Windows.h>
#include <time.h>

using namespace std;


// 消息类型
#define MSG_TYPE_SESSION            "Sysm.001.01"        // 会话消息
#define MSG_TYPE_SESSION_R          "Sysm.002.01"        // 会话回执
#define MSG_TYPE_OPEN_ACCT          "Acmt.001.01"        // 开户
#define MSG_TYPE_OPEN_ACCT_R        "Acmt.002.01"        // 开户回执
#define MSG_TYPE_CLOSE_ACCT         "Acmt.003.01"        // 销户消息
#define MSG_TYPE_CLOSE_ACCT_R       "Acmt.004.01"        // 销户回执
#define MSG_TYPE_MODIFY_ACCT        "Acmt.005.01"        // 账户修改
#define MSG_TYPE_MODIFY_ACCT_R      "Acmt.006.01"        // 账户修改回执
#define MSG_TYPE_UPD_ACCT           "Acmt.007.01"        // 账户变更
#define MSG_TYPE_UPD_ACCT_R         "Acmt.008.01"        // 账户变更回执
#define MSG_TYPE_QUERY_ACCT         "Acmt.009.01"        // 账户查询
#define MSG_TYPE_QUERY_ACCT_R       "Acmt.010.01"        // 账户查询回执
#define MSG_TYPE_TRADE              "Trf.001.01"         // 转账
#define MSG_TYPE_TRADE_R            "Trf.002.01"         // 转账回执
#define MSG_TYPE_UNTRADE            "Trf.003.01"         // 转账冲正
#define MSG_TYPE_UNTRADE_R          "Trf.004.01"         // 转账冲正回执
#define MSG_TYPE_QRY_TRD_RST        "Trf.005.01"         // 交易结果查询
#define MSG_TYPE_QRY_TRD_RST_R      "Trf.006.01"         // 交易结果查询回执
#define MSG_TYPE_SETT_WELL          "Trf.007.01"         // 结息
#define MSG_TYPE_SETT_WELL_R        "Trf.008.01"         // 结息回执
#define MSG_TYPE_CHECK_SEQ          "Stmt.001.01"        // 对账
#define MSG_TYPE_CHECK_SEQ_R        "Stmt.002.01"        // 对账回执
#define MSG_TYPE_SETTFILE_OK        "Stmt.003.01"        // 日终数据就绪
#define MSG_TYPE_SETTFILE_OK_R      "Stmt.004.01"        // 日终数据就绪回执
#define MSG_TYPE_FILE_OPER          "File.001.01"        // 文件操作
#define MSG_TYPE_FILE_OPER_R        "File.002.01"        // 文件操作回执

// 帮助函数
// 获取当时时间信息
std::string MGetCurDate(const char *format);
// 获取流水号
std::string MGetLsh();
// 生成银行卡号
std::string MMakeBkAcct(const string& strScAcct, const string& strBkAcct);
// 组装XML标签
std::string MMakeXmlElem(const string& strTag, const string& strText);
// 计算校验和
int MCalCheckSum(const string& strBody);
// 获取元素对象
const tinyxml2::XMLElement* XmlFindFirst(const tinyxml2::XMLElement* pe, const string& strName);
// 获取元素内容
std::string XmlTextFirst(const tinyxml2::XMLElement* pe, const string& strName);

// 消息组件
class CMsgComponent
{
public:
    // 存储XML组件的子项字典值，KEY为标签值，VAL为标签内容
    map<string, string>    _map_val;

    // 解析组件XML数据-字符串
    virtual bool Parse(const string& strXml);
    // tinyxml2元素对象
    virtual bool Parse(const tinyxml2::XMLElement* pe);
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CMsgComponent(*this);
    }
    // 打包对象为XML数据
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

// 通讯消息类型
class CTxMsg
{
public:
    // 业务要素组件
    map<string, CMsgComponent*> m_mapComponent;
    // 根节点属性
    map<string, string>         m_mapRootAttr;
    // 消息类型标签
    string                      m_strMsgType;

public:
    // 构造函数
    CTxMsg(){}
    // 拷贝构造函数
    CTxMsg(const CTxMsg& m)
    {
        m.CopyTo(this);
    }
    // 重载拷贝构造函数
    CTxMsg & operator= (const CTxMsg& m)
    {
        m.CopyTo(this);
        return *this;
    }
    // 克隆对象本身
    void CopyTo(CTxMsg* p) const
    {
        if(!p) return;
        // 普通变量拷贝
        p->m_strMsgType = m_strMsgType;
        p->m_mapRootAttr = m_mapRootAttr;
        // 深拷贝
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
    // 析构函数
    ~CTxMsg();
    // 消息解析
    bool Parse(const string& strXml);
    // 消息组包
    string PackXml();

    // 消息解析
    bool Parse(tinyxml2::XMLElement* pr);
    // 解析消息组件
    bool ParseComponents(tinyxml2::XMLElement* pb);
    // 获取组件
    CMsgComponent* GetComponent(const string& strName);
    // 添加组件
    void AddComponent(const string& strName, CMsgComponent* p);
    // 获取应答包类型
    string GetRetMsgType() const;
    // 判断是否为请求消息
    static bool IsReqMsg(const string& strXml);
    // 从文件读取消息
    static int LoadMsg(const string& strPath, vector<string>& vecXml);

public:
    // 内容清空
    void Clear();
};

#define MSG_SRC_SEND     100
#define MSG_SRC_FILE     101

// 请求消息内容
class CMsgInfo
{
public:
    // 消息内容
    const char*     psPkg;
    // 消息长度
    int             iPkgLen;
    // 消息对象
    CTxMsg          oPkg;
    // 消息属性
    STUMsgProperty  oMsgPropery;
    // 超时时间
    int             iMillSecTimeo;
    // 消息来源
    int             nMsgSrc;
};

// 自定义类型前置声明
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

// 8.2　返回结果（ReturnResult）
class CReturnResult: public CMsgComponent
{
public:
    string                      strCode;                         // 返回码 <Code>	[1..1]	ReturnCode
    string                      strInfo;                         // 返回信息 <Info>	[0..1]	Max128Text


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CReturnResult(*this);
    }
    // 打包对象为XML数据
    virtual string PackXml()
    {
        string xml;

        xml += "<Code>" + strCode + "</Code>\n";
        xml += "<Info>" + strInfo + "</Info>\n";

        return xml;
    }
    // tinyxml2元素对象
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strCode = XmlTextFirst(pe, "Code");
        strInfo = XmlTextFirst(pe, "Info");
        return true;
    }
};

// 8.3　流水号(Reference)
class CReference: public CMsgComponent
{
public:
    string                      strReference;                    // 流水号 <Ref>	[1..1]	Max35Text
    string                      strRefrenceIssureType;           // 流水号发布者类型 <IssrType>	[1..1]	InstitutionType
    string                      strReferenceIssure;              // 发布者 <RefIssr>	[0..1]	Max35Text


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CReference(*this);
    }
    // 打包对象为XML数据
    virtual string PackXml()
    {
        string xml;

        xml += "<Ref>" + strReference + "</Ref>\n";
        xml += "<IssrType>" + strRefrenceIssureType + "</IssrType>\n";
        xml += "<RefIssr>" + strReferenceIssure + "</RefIssr>\n";

        return xml;
    }
    // tinyxml2元素对象
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strReference = XmlTextFirst(pe, "Ref");
        strRefrenceIssureType = XmlTextFirst(pe, "IssrType");
        strReferenceIssure = XmlTextFirst(pe, "RefIssr");
        return true;
    }
};

// 8.4　机构信息（Institution）
class CInstitution: public CMsgComponent
{
public:
    string                      strInstitutionType;              // 机构类型 <InstType>	[1..1]	InstitutionType
    string                      strInstitutionIdentifier;        // 机构标识 <InstId>	[1..1]	Max35Text
    string                      strInstitutionName;              // 机构名称 <InstNm>	[0..1]	Max70Text
    string                      strBranchIdentifier;             // 分支机构编码 <BrchId>	[0..1]	Max35Text
    string                      strBranchName;                   // 分支机构名称 <BrchNm>	[0..1]	Max70Text
    string                      strSubBranchIdentifier;          // 网点号 <SubBrchId>	[0..1]	Max35Text
    string                      strSubBranchName;                // 网点名称 <SubBrchNm>	[0..1]	Max70Text


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CInstitution(*this);
    }
    // 打包对象为XML数据
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
    // tinyxml2元素对象
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

// 8.5　客户信息（Customer）
class CCustomer: public CMsgComponent
{
public:
    string                      strName;                         // 客户姓名 <Name>	[1..1]	Max70Text
    string                      strCertificationType;            // 证件类型 <CertType>	[1..1]	CertificationType
    string                      strCertificationIdentifier;      // 证件号码 <CertId>	[1..1]	Max35Text
    string                      strType;                         // 客户类型 <Type>	[0..1]	CustomerType
    string                      strGender;                       // 客户性别 <Gender>	[0..1]	GenderCode
    string                      strNationality;                  // 客户国籍 <Ntnl>	[0..1]	CountryCode
    string                      strAddress;                      // 通信地址 <Addr>	[0..1]	Max70Text
    string                      strPostcode;                     // 邮政编码 <PstCd>	[0..1]	Max35Text
    string                      strEmail;                        // 电子邮件 <Email>	[0..1]	Max70Text
    string                      strFax;                          // 传真 <Fax>	[0..1]	Max35Text
    string                      strMobile;                       // 手机 <Mobile>	[0..1]	Max35Text
    string                      strTelephone;                    // 电话 <Tel>	[0..1]	Max35Text
    string                      strBp;                           // 传呼 <Bp>	[0..1]	Max35Text


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CCustomer(*this);
    }
    // 打包对象为XML数据
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
    // tinyxml2元素对象
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

// 8.6　代理人信息（Agent）
class CAgent: public CMsgComponent
{
public:
    string                      strName;                         // 代理人姓名 <Name>	[0..1]	Max70Text
    string                      strCertificationType;            // 证件类型 <CertType>	[0..1]	CertificationType
    string                      strCertificationIdentifier;      // 证件号码 <CertId>	[0..1]	Max35Text
    string                      strAuthentication;               // 代理人权限 <Auth>	[0..1]	AgentAuthCode
    string                      strGender;                       // 代理人性别 <Gender>	[0..1]	GenderCode
    string                      strNationality;                  // 代理人国籍 <Ntnl>	[0..1]	CountryCode
    string                      strAddress;                      // 通信地址 <Addr>	[0..1]	Max70Text
    string                      strPostcode;                     // 邮政编码 <PstCd>	[0..1]	Max35Text
    string                      strEmail;                        // 电子邮件 <Email>	[0..1]	Max70Text
    string                      strFax;                          // 传真 <Fax>	[0..1]	Max35Text
    string                      strMobile;                       // 手机 <Mobile>	[0..1]	Max35Text
    string                      strTelephone;                    // 电话 <Tel>	[0..1]	Max35Text
    string                      strBp;                           // 传呼 <Bp>	[0..1]	Max35Text
    string                      strBeginDate;                    // 代理开始日 <BgnDt>	[0..1]	Date
    string                      strEndDate;                      // 代理到期日 <EndDt>	[0..1]	Date


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CAgent(*this);
    }
    // 打包对象为XML数据
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
    // tinyxml2元素对象
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

// 8.8　密码（Password）
class CPassword: public CMsgComponent
{
public:
    string                      strType;                         // 密码类型 <Type>	[0..1]	PasswordType
    string                      strEncryMode;                    // 加密方式 <Enc>	[0..1]	EncryMode
    string                      strPassword;                     // 密码 <Pwd>	[0..1]	Max35Text


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CPassword(*this);
    }
    // 打包对象为XML数据
    virtual string PackXml()
    {
        string xml;

        xml += "<Type>" + strType + "</Type>\n";
        xml += "<Enc>" + strEncryMode + "</Enc>\n";
        xml += "<Pwd>" + strPassword + "</Pwd>\n";

        return xml;
    }
    // tinyxml2元素对象
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strType = XmlTextFirst(pe, "Type");
        strEncryMode = XmlTextFirst(pe, "Enc");
        strPassword = XmlTextFirst(pe, "Pwd");
        return true;
    }
};

// 8.9　余额(Balance)
class CBalance: public CMsgComponent
{
public:
    string                      strBalanceType;                  // 余额类型 <Type>	[0..1]	BalanceType
    string                      strBalance;                      // 余额 <Bal>	[1..1]	Amount


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CBalance(*this);
    }
    // 打包对象为XML数据
    virtual string PackXml()
    {
        string xml;

        xml += "<Type>" + strBalanceType + "</Type>\n";
        xml += "<Bal>" + strBalance + "</Bal>\n";

        return xml;
    }
    // tinyxml2元素对象
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        strBalanceType = XmlTextFirst(pe, "Type");
        strBalance = XmlTextFirst(pe, "Bal");
        return true;
    }
};

// 8.10　文件信息(FileInfo)
class CFileInfo: public CMsgComponent
{
public:
    string                      strFileBusCode;                  // 文件业务功能 <BusCode>	[0..1]	FileBusinessCode
    string                      strBusinessDate;                 // 文件业务日期 <BusDate>	[0..1]	Date
    string                      strHost;                         // 文件存放主机 <Host>	[0..1]	Max35Text
    string                      strFileName;                     // 文件名称 <FileName>	[1..1]	Max128Text
    string                      strFileLength;                   // 文件长度 <FileLen>	[0..1]	Number
    string                      strFileTime;                     // 文件时间 <FileTime>	[0..1]	DateTime
    string                      strFileMac;                      // 文件校验码 <FileMac>	[0..1]	Max128Text


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CFileInfo(*this);
    }
    // 打包对象为XML数据
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
    // tinyxml2元素对象
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

// 8.1　消息头（MessgeHeader）
class CMessgeHeader: public CMsgComponent
{
public:
    string                      strVersion;                      // 版本 <Ver>	[1..1]	Max35Text
    string                      strSystemType;                   // 应用系统类型 <SysType>	[1..1]	SystemType
    string                      strInstructionCode;              // 业务功能码 <InstrCd>	[1..1]	InstructionCode
    string                      strTradeSource;                  // 交易发起方 <TradSrc>	[1..1	InstitutionType
    CInstitution                oSender;                         // 发送机构 <Sender>	[1..1]	Institution
    CInstitution                oRecver;                         // 接收机构 <Recver>	[1..1]	Institution
    string                      strCreateDate;                   // 发生日期 <Date>	[0..1]	Date
    string                      strCreateTime;                   // 发生时间 <Time>	[0..1]	Time
    CReference                  oRefrence;                       // 消息流水号 <Ref>	[0..1]	Reference
    CReference                  oRelatedReference;               // 相关消息流水号 <RltdRef>	[0..1]	Reference
    string                      strLastFragment;                 // 最后分片标志 <LstFrag>	[0..1]	YesNoIndicator


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CMessgeHeader(*this);
    }
    // 打包对象为XML数据
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
    // tinyxml2元素对象
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

// 8.7　账户<Account>
class CAccount: public CMsgComponent
{
public:
    string                      strAccountIdentification;        // 账号 <Id>	[1..1]	Max35Text
    string                      strAccountName;                  // 户主名称 <Name>	[0..1]	Max70Text
    string                      strAccountType;                  // 账户类别 <Type>	[0..1]	AccountType
    string                      strAccountStatus;                // 账户状态 <Status>	[0..1]	AccountStatus
    CPassword                   oPassword;                       // 密码 <Pwd>	[0..n]	Password
    string                      strRegisterDate;                 // 开户日期 <RegDt>	[0..1]	Date
    string                      strValidDate;                    // 有效日期 <VldDt>	[0..1]	Date
    CInstitution                oAccountService;                 // 账户服务商 <AcctSvcr>	[0..1]	Institution


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CAccount(*this);
    }
    // 打包对象为XML数据
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
    // tinyxml2元素对象
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

// 8.11　账户状态明细（AccountStatusStatement）
class CAccountStatusStatement: public CMsgComponent
{
public:
    CCustomer                   oCustomer;                       // 客户信息 <Cust>	[1..1]	Customer
    CAccount                    oBankAccount;                    // 银行账号 <BkAcct>	[0..1]	Account
    CAccount                    oSecuritiesAccount;              // 证券方账户 <ScAcct>	[1..1]	Account
    string                      strManageStatus;                 // 存管状态 <MngSt>	[1..1]	ManagerStatus
    string                      strCurrency;                     // 币种 <Ccy>	[0..1]	CurrencyCode
    string                      strCashExCode;                   // 汇钞标志 <CashExCd>	[0..1]	CashExCode
    string                      strDate;                         // 发生日期 <Date>	[0..1]	Date


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CAccountStatusStatement(*this);
    }
    // 打包对象为XML数据
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
    // tinyxml2元素对象
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

// 8.12　账户状态对账结果（AccountStatusStatementConfirm）
class CAccountStatusStatementConfirm: public CMsgComponent
{
public:
    CAccountStatusStatement     oBankEntry;                      // 银行方账户状态明细记录 <BkEntry>	[1..1]	AccountStatusStatement
    string                      strSecuritiesEntry;              // 证券方账户状态明细记录 <ScEntry>	[1..1]	AccountStatusStatementt
    CReturnResult               oCheckResult;                    // 对账结果 <ChkRst>	[1..1]	ReturnResult


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CAccountStatusStatementConfirm(*this);
    }
    // 打包对象为XML数据
    virtual string PackXml()
    {
        string xml;

        xml += "<BkEntry>" + oBankEntry.PackXml() + "</BkEntry>\n";
        xml += "<ScEntry>" + strSecuritiesEntry + "</ScEntry>\n";
        xml += "<ChkRst>" + oCheckResult.PackXml() + "</ChkRst>\n";

        return xml;
    }
    // tinyxml2元素对象
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        oBankEntry.Parse(XmlFindFirst(pe, "BkEntry"));
        strSecuritiesEntry = XmlTextFirst(pe, "ScEntry");
        oCheckResult.Parse(XmlFindFirst(pe, "ChkRst"));
        return true;
    }
};

// 8.13　账户交易明细（AccountTradeStatement）
class CAccountTradeStatement: public CMsgComponent
{
public:
    CReference                  oReference;                      // 流水号 <Ref>	[0..2]	Reference
    string                      strTradeSource;                  // 交易发起方 <TradSrc>	[0..1]	InstitutionType
    string                      strInstructionCode;              // 业务功能码 <InstrCd>	[0..1]	InstructionCode
    CCustomer                   oCustomer;                       // 客户信息 <Cust>	[0..1]	Customer
    CAccount                    oBankAccount;                    // 银行方账户 <BkAcct>	[0..1]	Account
    CAccount                    oSecuritiesAccount;              // 证券方账户 <ScAcct>	[0..1]	Account
    string                      strCurrency;                     // 币种 <Ccy>	[0..1]	CurrencyCode
    string                      strCashExCode;                   // 汇钞标志 <CashExCd>	[0..1]	CashExCode
    CBalance                    oSecuritiesBalance;              // 证券方账户余额 <ScBal>	[0..n]	Balance
    string                      strDate;                         // 发生日期 <Date>	[0..1]	Date
    string                      strTime;                         // 发生时间 <Time>	[0..1]	Time
    string                      strDigest;                       // 业务摘要 <Dgst>	[0..1]	Max35Text


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CAccountTradeStatement(*this);
    }
    // 打包对象为XML数据
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
    // tinyxml2元素对象
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

// 8.14　账户交易对账结果（AccountTradeStatementConfirm）
class CAccountTradeStatementConfirm: public CMsgComponent
{
public:
    CAccountTradeStatement      oBankEntry;                      // 银行方账户交易明细 <BkEntry>	[1..1]	AccountTradeStatement
    CAccountTradeStatement      oSecuritiesEntry;                // 银行方账户交易明细 <ScEntry>	[1..1]	AccountTradeStatement
    CReturnResult               oCheckResult;                    // 对账结果 <ChkRst>	[1..1]	ReturnResult


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CAccountTradeStatementConfirm(*this);
    }
    // 打包对象为XML数据
    virtual string PackXml()
    {
        string xml;

        xml += "<BkEntry>" + oBankEntry.PackXml() + "</BkEntry>\n";
        xml += "<ScEntry>" + oSecuritiesEntry.PackXml() + "</ScEntry>\n";
        xml += "<ChkRst>" + oCheckResult.PackXml() + "</ChkRst>\n";

        return xml;
    }
    // tinyxml2元素对象
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        oBankEntry.Parse(XmlFindFirst(pe, "BkEntry"));
        oSecuritiesEntry.Parse(XmlFindFirst(pe, "ScEntry"));
        oCheckResult.Parse(XmlFindFirst(pe, "ChkRst"));
        return true;
    }
};

// 8.15　转账明细(TransferStatement)
class CTransferStatement: public CMsgComponent
{
public:
    CReference                  oReference;                      // 流水号 <Ref>	[1..2]	Reference
    string                      strTradeSource;                  // 交易发起方 <TradSrc>	[1..1]	InstitutionType
    string                      strInstructionCode;              // 业务功能码 <InstrCd>	[1..1]	InstructionCode
    CCustomer                   oCustomer;                       // 客户信息 <Cust>	[0..1]	Customer
    CAccount                    oBankAccount;                    // 银行方账户 <BkAcct>	[1..1]	Account
    CAccount                    oSecuritiesAccount;              // 证券方账户 <ScAcct>	[1..1]	Account
    string                      strCashExCode;                   // 汇钞标志 <CashExCd>	[0..1]	CashExCode
    string                      strCurrency;                     // 币种 <Ccy>	[0..1]	CurrencyCode
    string                      strAmount;                       // 发生金额 <Amt>	[1..1]	Amount
    string                      strDate;                         // 发生日期 <Date>	[0..1]	Date
    string                      strTime;                         // 发生时间 <Time>	[0..1]	Time
    string                      strDigest;                       // 业务摘要 <Dgst>	[0..1]	Max35Text


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CTransferStatement(*this);
    }
    // 打包对象为XML数据
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
    // tinyxml2元素对象
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

// 8.16　转账对账结果(TransferStatementConfirm)
class CTransferStatementConfirm: public CMsgComponent
{
public:
    CTransferStatement          oBankEntry;                      // 银行方转账明细 <BkEntry>	[0..1]	TransferStatement
    CTransferStatement          oSecuritiesEntry;                // 证券方转账明细 <ScEntry>	[0..1]	TransferStatement
    CReturnResult               oCheckResult;                    // 对账结果 <ChkRst>	[1..1]	ReturnResult


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CTransferStatementConfirm(*this);
    }
    // 打包对象为XML数据
    virtual string PackXml()
    {
        string xml;

        xml += "<BkEntry>" + oBankEntry.PackXml() + "</BkEntry>\n";
        xml += "<ScEntry>" + oSecuritiesEntry.PackXml() + "</ScEntry>\n";
        xml += "<ChkRst>" + oCheckResult.PackXml() + "</ChkRst>\n";

        return xml;
    }
    // tinyxml2元素对象
    virtual bool Parse(const tinyxml2::XMLElement* pe)
    {
        if(!pe) return false;
        oBankEntry.Parse(XmlFindFirst(pe, "BkEntry"));
        oSecuritiesEntry.Parse(XmlFindFirst(pe, "ScEntry"));
        oCheckResult.Parse(XmlFindFirst(pe, "ChkRst"));
        return true;
    }
};

// 8.17　账户余额明细(BalanceStatement)
class CBalanceStatement: public CMsgComponent
{
public:
    CCustomer                   oCustomer;                       // 客户信息 <Cust>	[0..1]	Customer
    CAccount                    oBankAccount;                    // 银行方账户 <BkAcct>	[0..1]	Account
    CAccount                    oSecuritiesAccount;              // 证券方账户 <ScAcct>	[0..1]	Account
    string                      strCashExCode;                   // 汇钞标志 <CashExCd>	[0..1]	CashExCode
    string                      strCurrency;                     // 币种 <Ccy>	[0..1]	CurrencyCode
    CBalance                    oSecuritiesBalance;              // 资金账户余额 <ScBal>	[0..n]	Balance
    string                      strDate;                         // 发生日期 <Date>	[0..1]	Date


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CBalanceStatement(*this);
    }
    // 打包对象为XML数据
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
    // tinyxml2元素对象
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

// 8.18　账户余额对账结果(BalanceStatementConfirm)
class CBalanceStatementConfirm: public CMsgComponent
{
public:
    CBalanceStatement           oBankEntry;                      // 银行方账户余额明细 <BkEntry>	[1..1]	BalanceStatement
    CBalanceStatement           oSecuritiesEntry;                // 证券方账户余额明细 <ScEntry>	[1..1]	BalanceStatement
    CReturnResult               oCheckResult;                    // 对账结果 <ChkRst>	[0..1]	ReturnResult


public:
    // 深拷贝函数
    virtual CMsgComponent* Clone()
    {
        return new CBalanceStatementConfirm(*this);
    }
    // 打包对象为XML数据
    virtual string PackXml()
    {
        string xml;

        xml += "<BkEntry>" + oBankEntry.PackXml() + "</BkEntry>\n";
        xml += "<ScEntry>" + oSecuritiesEntry.PackXml() + "</ScEntry>\n";
        xml += "<ChkRst>" + oCheckResult.PackXml() + "</ChkRst>\n";

        return xml;
    }
    // tinyxml2元素对象
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
    string      m_strRetCode;   // 返回代码
    string      m_strRetInfo;   // 返回信息
    string      m_BankReqFile;  // 银行请求数据
    string      m_BankAnsFile;  // 银行应答数据

private:
    void LoadConfig();

private:
    bool MakeRespPkgSession(CMsgInfo& oReq, CMsgInfo& oResp);
    bool MakeRespPkgTrade(CMsgInfo& oReq, CMsgInfo& oResp);
    bool MakeRespPkgOpenAcct(CMsgInfo& oReq, CMsgInfo& oResp);
};