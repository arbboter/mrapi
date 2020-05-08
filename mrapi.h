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

#define MR_PROTOCOLTYPE_MRSTANDAND     0x00    //��ϵͳ֧�ֵı�׼Э��
#define MR_PROTOCOLTYPE_SELFCUSTOM     0xFF    //�û��Զ�������Э��


#define MR_MSGFLAG_PERSIST          0x01   //�־���Ϣ��־
#define MR_MSGFLAG_COMPRESS         0x02   //ѹ����־


#define MR_MAXLEN_ADDR              64     //Դ��ַ��Ŀ�ĵ�ַ����󳤶�
#define MR_MAXLEN_PKGID             64     //��ID����󳤶�(Ŀǰ�汾��ʵ�ʳ���Ϊ36)
#define MR_MAXLEN_USERDATA          256    //�û����ݵ���󳤶�
#define MR_FIXLEN_EXPIREDABSTIME    20     //����ʱ��Ĺ̶�����


#ifdef __cplusplus  
extern "C" {  
#endif  


    struct STUMsgProperty
    {
        char                m_szSourceUserID[MR_MAXLEN_ADDR];     //Դ�û���ʶ��������'\0'��β���ַ�����
        char                m_szSourceAppID[MR_MAXLEN_ADDR];     //ԴӦ�ñ�ʶ��������'\0'��β���ַ�����
        char                m_szDestUserID[MR_MAXLEN_ADDR];       //Ŀ���û���ʶ��������'\0'��β���ַ�����
        char                m_szDestAppID[MR_MAXLEN_ADDR];       //Ŀ��Ӧ�ñ�ʶ��������'\0'��β���ַ�����
        char                m_szPkgID[MR_MAXLEN_PKGID];         //��ID, ������'\0'��β���ַ���. �������û�ʹ��MrCreatePkgID��������,�������
        char                m_szCorrPkgID[MR_MAXLEN_PKGID];     //��ذ�ID, ������'\0'��β���ַ���, ���û�ʹ��
        char                m_szUserData1[MR_MAXLEN_USERDATA];  //�û�����1, ������'\0'��β���ַ���, ���û�ʹ��
        char                m_szUserData2[MR_MAXLEN_USERDATA];  //�û�����2, ������'\0'��β���ַ���, ���û�ʹ��
        char                m_szExpiredAbsTime[MR_FIXLEN_EXPIREDABSTIME];   //����ʱ��(����ʱ���ʾ). ������'\0'��β���ַ���. ��ʽΪYYYY-MM-DD hh:mm:ss. ����2006-09-21 03:45:00,Ĭ�Ϲ���ʱ��Ϊ����23:59:59.
        // ��Ϊ��ʱ������Է��û������ߣ����߶Է�Ӧ��δ���ӣ����������ڡ� 
        unsigned char       m_ucFlag;      //��־:������MR_MSGFLAG_PERSIST��MR_MSGFLAG_COMPRESS�ȱ�־��λ��.
        unsigned char       m_ucProtocolType;      //Э������:������MR_PROTOCOLTYPE_MRSTANDAND��MR_PROTOCOLTYPE_SELFCUSTOM֮һ.
    };

    struct STUConnInfo
    {
        char                m_szMRIP[16];       //����MR��IP. ������'\0'��β���ַ���. "127.0.0.1"
        unsigned short      m_usMRPort;         //����MR�Ķ˿�.    51231
        char                m_szMRIPBak[16];    //����MR��IP. ������'\0'��β���ַ���. ����ʱ,����Ϊ��
        unsigned short      m_usMRPortBak;      //����MR�Ķ˿�.  ����ʱ,����Ϊ0
    };

#define CALL_METHOD _stdcall 

    typedef int (*RecallOnReceive)(const char* psPkg, int iPkgLen, const STUMsgProperty* pMsgPropery, void* pvUserData);

    MRAPI_API void*  CALL_METHOD MrInit(const char* psAppID, const char* psPasswd,RecallOnReceive pOnReceive,const STUConnInfo oConnInfo, void* pvUserData);
    MRAPI_API void*  CALL_METHOD MrInit1(const char* psUserCertID, const char* psAppID, const char* psUserPasswd,RecallOnReceive pOnReceive, const STUConnInfo oConnInfo, void* pvUserData);
    MRAPI_API int    CALL_METHOD MrCreatePkgID(void* pHandle, char szPkgID[MR_MAXLEN_PKGID]);
    MRAPI_API int    CALL_METHOD MrSend(void* pHandle, const char* psPkg, int iPkgLen, STUMsgProperty* pMsgPropery, int iMillSecTimeo);
    MRAPI_API int    CALL_METHOD MrBrowse(void* pHandle, int* piOutPkgLen, STUMsgProperty* pMsgPropery, int iMillSecTimeo);  /*�����һ����*/
    MRAPI_API int    CALL_METHOD MrReceive1(void* pHandle, char** ppsPkg, int* piOutPkgLen, STUMsgProperty* pMsgPropery, int iMillSecTimeo);  
    MRAPI_API void   CALL_METHOD MrReceive1_FreeBuf(char* psPkg);
    MRAPI_API int    CALL_METHOD MrReceive2(void* pHandle, char* psPkg, int* piOutPkgLen, int iBufLenIn, STUMsgProperty* pMsgPropery, int iMillSecTimeo);  
    MRAPI_API int    CALL_METHOD MrReceive3(void* pHandle, char** ppsPkg, int* piOutPkgLen, int* piErrSXCode, STUMsgProperty* pMsgPropery, int iMillSecTimeo);  
    MRAPI_API int    CALL_METHOD MrIsLinkOK(void* pHandle);  
    MRAPI_API void   CALL_METHOD MrDestroy(void* pHandle);
    MRAPI_API void   CALL_METHOD MrUninit(void* pHandle);
    MRAPI_API void   CALL_METHOD ShowMsg();

//�����C�������ļ�����extern "C"����
#ifdef __cplusplus  
}  
#endif
#endif