// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MRAPI_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MRAPI_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef __MRAPI_H__
#define __MRAPI_H__
#ifdef MRAPI_EXPORTS
#define MRAPI_API __declspec(dllexport)
#else
#define MRAPI_API __declspec(dllimport)
#endif

#define MR_PROTOCOLTYPE_MRSTANDAND     0x00    //本系统支持的标准协议
#define MR_PROTOCOLTYPE_SELFCUSTOM     0xFF    //用户自定义类型协议


#define MR_MSGFLAG_PERSIST          0x01   //持久消息标志
#define MR_MSGFLAG_COMPRESS         0x02   //压缩标志


#define MR_MAXLEN_ADDR              64     //源地址或目的地址的最大长度
#define MR_MAXLEN_PKGID             64     //包ID的最大长度(目前版本中实际长度为36)
#define MR_MAXLEN_USERDATA          256    //用户数据的最大长度
#define MR_FIXLEN_EXPIREDABSTIME    20     //过期时间的固定长度


#ifdef __cplusplus  
extern "C" {  
#endif  


    struct STUMsgProperty
    {
        char                m_szSourceUserID[MR_MAXLEN_ADDR];     //源用户标识，必须是'\0'结尾的字符串。
        char                m_szSourceAppID[MR_MAXLEN_ADDR];     //源应用标识，必须是'\0'结尾的字符串。
        char                m_szDestUserID[MR_MAXLEN_ADDR];       //目的用户标识，必须是'\0'结尾的字符串。
        char                m_szDestAppID[MR_MAXLEN_ADDR];       //目的应用标识，必须是'\0'结尾的字符串。
        char                m_szPkgID[MR_MAXLEN_PKGID];         //包ID, 必须是'\0'结尾的字符串. 或者由用户使用MrCreatePkgID函数生成,或者填空
        char                m_szCorrPkgID[MR_MAXLEN_PKGID];     //相关包ID, 必须是'\0'结尾的字符串, 供用户使用
        char                m_szUserData1[MR_MAXLEN_USERDATA];  //用户数据1, 必须是'\0'结尾的字符串, 供用户使用
        char                m_szUserData2[MR_MAXLEN_USERDATA];  //用户数据2, 必须是'\0'结尾的字符串, 供用户使用
        char                m_szExpiredAbsTime[MR_FIXLEN_EXPIREDABSTIME];   //过期时间(绝对时间表示). 必须是'\0'结尾的字符串. 格式为YYYY-MM-DD hh:mm:ss. 例如2006-09-21 03:45:00,默认过期时间为当天23:59:59.
        // 当为空时，如果对方用户不在线，或者对方应用未连接，则立即过期。 
        unsigned char       m_ucFlag;      //标志:可以是MR_MSGFLAG_PERSIST或MR_MSGFLAG_COMPRESS等标志的位或.
        unsigned char       m_ucProtocolType;      //协议类型:可以是MR_PROTOCOLTYPE_MRSTANDAND或MR_PROTOCOLTYPE_SELFCUSTOM之一.
    };

    struct STUConnInfo
    {
        char                m_szMRIP[16];       //主用MR的IP. 必须是'\0'结尾的字符串. "127.0.0.1"
        unsigned short      m_usMRPort;         //主用MR的端口.    51231
        char                m_szMRIPBak[16];    //备用MR的IP. 必须是'\0'结尾的字符串. 不用时,可以为空
        unsigned short      m_usMRPortBak;      //备用MR的端口.  不用时,可以为0
    };

#define CALL_METHOD _stdcall 

    typedef int (*RecallOnReceive)(const char* psPkg, int iPkgLen, const STUMsgProperty* pMsgPropery, void* pvUserData);

    MRAPI_API void*  CALL_METHOD MrInit(const char* psAppID, const char* psPasswd,RecallOnReceive pOnReceive,const STUConnInfo oConnInfo, void* pvUserData);
    MRAPI_API void*  CALL_METHOD MrInit1(const char* psUserCertID, const char* psAppID, const char* psUserPasswd,RecallOnReceive pOnReceive, const STUConnInfo oConnInfo, void* pvUserData);
    MRAPI_API int    CALL_METHOD MrCreatePkgID(void* pHandle, char szPkgID[MR_MAXLEN_PKGID]);
    MRAPI_API int    CALL_METHOD MrSend(void* pHandle, const char* psPkg, int iPkgLen, STUMsgProperty* pMsgPropery, int iMillSecTimeo);
    MRAPI_API int    CALL_METHOD MrBrowse(void* pHandle, int* piOutPkgLen, STUMsgProperty* pMsgPropery, int iMillSecTimeo);  /*浏览下一个包*/
    MRAPI_API int    CALL_METHOD MrReceive1(void* pHandle, char** ppsPkg, int* piOutPkgLen, STUMsgProperty* pMsgPropery, int iMillSecTimeo);  
    MRAPI_API void   CALL_METHOD MrReceive1_FreeBuf(char* psPkg);
    MRAPI_API int    CALL_METHOD MrReceive2(void* pHandle, char* psPkg, int* piOutPkgLen, int iBufLenIn, STUMsgProperty* pMsgPropery, int iMillSecTimeo);  
    MRAPI_API int    CALL_METHOD MrReceive3(void* pHandle, char** ppsPkg, int* piOutPkgLen, int* piErrSXCode, STUMsgProperty* pMsgPropery, int iMillSecTimeo);  
    MRAPI_API int    CALL_METHOD MrIsLinkOK(void* pHandle);  
    MRAPI_API void   CALL_METHOD MrDestroy(void* pHandle);
    MRAPI_API void   CALL_METHOD MrUninit(void* pHandle);
    MRAPI_API void   CALL_METHOD ShowMsg();

//如果用C编译器文件，用extern "C"声明
#ifdef __cplusplus  
}  
#endif
#endif