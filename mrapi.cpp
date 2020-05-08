// mrapi.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "mrapi.h"
#include "IFTS.h"
#include <time.h>
#include <Windows.h>
#include <stdio.h>
#include <map>
#include <string>
#include <sstream>
#include <queue>

using namespace std;

// ������Ϣ
class CConInfo
{
public:
    string              strAppId;
    string              strPasswd;
    RecallOnReceive     pOnReceive;
    STUConnInfo         oConnInfo;
    void*               pvUserData;

private:
    // �ڲ�����
    // ������Ϣ
    queue<CMsgInfo>     qReqMsg;
    // �߳���
    CRITICAL_SECTION    csLock;
    // �ļ��޸�ʱ��
    map<string, FILETIME> mapFileTime;

    // �ļ��Ƿ���ˣ����޸�ʱ��
    bool IsFileChange(const string& strFile)
    {
        // ��ȡ�ļ���Ϣ
        FILETIME ftCreate, ftModify, ftAccess;

        HANDLE hFile = CreateFile(strFile.c_str(), GENERIC_READ,          // open for reading
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL);
        if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftModify))
        {
            return false;
        }

        // �鿴�ļ��Ƿ����
        FILETIME ftLastTime = {0};
        map<string, FILETIME>::iterator f = mapFileTime.find(strFile);
        if(f != mapFileTime.end())
        {
            ftLastTime = f->second;
        }
        mapFileTime[strFile] = ftModify;
        return CompareFileTime(&ftModify, &ftLastTime) > 0;
    }


public:
    CConInfo()
    {
        // ��ʼ���������ٽ����
        pOnReceive = NULL;
        pvUserData = NULL;
        InitializeCriticalSection(&csLock);
    }
    ~CConInfo()
    {
        // ɾ���ٽ����
        DeleteCriticalSection(&csLock);
    }

    // ������
    void Lock()
    {
        EnterCriticalSection(&csLock);
    }
    void Unlock()
    {
        LeaveCriticalSection(&csLock);
    }

    // ���ļ�������Ϣ
    void LoadFile()
    {
        // ��ȡ��������
        CIFTSResp oIfts;
        vector<string> vecXml;
        if(IsFileChange(oIfts.m_BankReqFile))
        {
            LOGD("��ȡ���������¼��[%d]", CTxMsg::LoadMsg(oIfts.m_BankReqFile, vecXml))
        }
        if(IsFileChange(oIfts.m_BankAnsFile))
        {
            LOGD("��ȡ����Ӧ���¼��[%d]", CTxMsg::LoadMsg(oIfts.m_BankAnsFile, vecXml))
        }

        // ������Ϣ
        CMsgInfo oInfo;
        char* pBuf = NULL;
        oInfo.iMillSecTimeo = 2000;
        oInfo.nMsgSrc = MSG_SRC_FILE;
        for (size_t i=0; i<vecXml.size(); i++)
        {
            oInfo.iPkgLen = vecXml[i].size();
            pBuf = new char[vecXml[i].size() + 1];
            memset(pBuf, 0, vecXml[i].size() + 1);
            strcpy(pBuf, vecXml[i].c_str());
            oInfo.psPkg = pBuf;
            Push(oInfo);
        }
    }

    // �������
    void Push(const CMsgInfo& oMsg)
    {
        Lock();
        qReqMsg.push(oMsg);
        Unlock();
    }
    // ȡ����
    bool GetPop(CMsgInfo& oMsg, int iMillSecTimeo=2000)
    {
        bool bRet = false;
        // ������ʱ��ʱ�ȴ�
        while(iMillSecTimeo>0 && qReqMsg.empty())
        {
            iMillSecTimeo -= 100;
        }
        // �����ݳ��Դ��ļ���������
        if(qReqMsg.empty())
        {
            LoadFile();
        }
        // ���Ի�ȡ��������
        if(!qReqMsg.empty())
        {
            Lock();
            if(!qReqMsg.empty())
            {
                oMsg = qReqMsg.front();
                qReqMsg.pop();
                bRet = true;
            }
            Unlock();
        }
        return bRet;
    }
};
map<string, CConInfo>        g_mapHandle;    // ������Ϣ

MRAPI_API void*  CALL_METHOD MrInit(const char* psAppID, const char* psPasswd,RecallOnReceive pOnReceive,const STUConnInfo oConnInfo, void* pvUserData)
{
    // �鿴�Ƿ��Ѵ���
    char* pHandle = const_cast<char*>(psAppID);
    if(psAppID && g_mapHandle.find(pHandle) != g_mapHandle.end())
    {
        return pHandle;
    }
    // ���ɾ��
    pHandle = new char[65];
    memset(pHandle, 0, 65);
    if(psAppID)
    {
        strncpy(pHandle, psAppID, 64);
    }
    else
    {
        for (int i=0; i<64; i++)
        {
            pHandle[i] = rand()%26 + 'A';
        }
    }
    // ����������Ϣ
    CConInfo oInfo;
    oInfo.strAppId = pHandle;
    oInfo.strPasswd = psPasswd;
    oInfo.pOnReceive = pOnReceive;
    oInfo.oConnInfo = oConnInfo;
    oInfo.pvUserData = pvUserData;

    // ���浽ȫ�ֱ���
    LOGI("��ʼ�����[%s]�ɹ�", pHandle);
    g_mapHandle[pHandle] = oInfo;
    return pHandle;
}

MRAPI_API void*  CALL_METHOD MrInit1(const char* psUserCertID, const char* psAppID, const char* psUserPasswd,RecallOnReceive pOnReceive, const STUConnInfo oConnInfo, void* pvUserData)
{
    return NULL;
}

MRAPI_API int     CALL_METHOD MrCreatePkgID(void* pHandle, char szPkgID[MR_MAXLEN_PKGID])
{
    int nRet = 0;
    return nRet;
}

MRAPI_API int  CALL_METHOD MrSend(void* pHandle, const char* psPkg, int iPkgLen, STUMsgProperty* pMsgPropery, int iMillSecTimeo)
{
    int nRet = 0;

    // ��ѯ����Ƿ����
    char* pName = (char*)pHandle;
    if(pHandle==NULL || g_mapHandle.find(pName) == g_mapHandle.end())
    {
        LOGE("���[%s]������", (pName ? pName:"Null"));
        return -1;
    }
    // �����Ϣ����
    if(psPkg==NULL || iPkgLen<=0)
    {
        LOGE("�������ݷǷ������ݲ�����Ϊ��");
        return -2;
    }
    // ��ȡ��Ϣ����
    CMsgInfo oMsg;
    char* pSendBuf = new char[iPkgLen+1];
    memset(pSendBuf, 0, iPkgLen+1);
    memcpy(pSendBuf, psPkg, iPkgLen);
    oMsg.psPkg = pSendBuf;
    oMsg.iPkgLen = iPkgLen;
    oMsg.nMsgSrc = MSG_SRC_SEND;
    if(pMsgPropery)
    {
        oMsg.oMsgPropery = *pMsgPropery;
    }
    oMsg.iMillSecTimeo = iMillSecTimeo;
    LOGI("�յ�������Ϣ,����[%d] ����[%s]", iPkgLen, psPkg);

    // ���˳��ΪӦ����Ϣ��ֱ�Ӷ���
    if(CTxMsg::IsReqMsg(pSendBuf))
    {
        LOGD("Ӧ����Ϣ���������Ѷ���");
        return 0;
    }

    // �������󵽱���
    CConInfo& oInfo = g_mapHandle[pName];
    oInfo.Push(oMsg);

    return nRet;
}

MRAPI_API int     CALL_METHOD MrBrowse(void* pHandle, int* piOutPkgLen, STUMsgProperty* pMsgPropery, int iMillSecTimeo)
{
    int nRet = 0;
    return nRet;
}
MRAPI_API int     CALL_METHOD MrReceive1(void* pHandle, char** ppsPkg, int* piOutPkgLen, STUMsgProperty* pMsgPropery, int iMillSecTimeo)
{
    int nRet = 0;
    return nRet;
}

MRAPI_API void    CALL_METHOD MrReceive1_FreeBuf(char* psPkg)
{
    delete[] psPkg;
    psPkg = NULL;
}
MRAPI_API int     CALL_METHOD MrReceive2(void* pHandle, char* psPkg, int* piOutPkgLen, int iBufLenIn, STUMsgProperty* pMsgPropery, int iMillSecTimeo)
{
    int nRet = 0;
    return nRet;
}

MRAPI_API int   CALL_METHOD MrReceive3(void* pHandle, char** ppsPkg, int* piOutPkgLen, int* piErrSXCode, STUMsgProperty* pMsgPropery, int iMillSecTimeo)
{
    int nRet = 0;

    // ��ѯ����Ƿ����
    char* pName = (char*)pHandle;
    map<string, CConInfo>::iterator e = g_mapHandle.find(pName);
    if(pName==NULL || e==g_mapHandle.end())
    {
        LOGE("���[%s]������", (pName ? pName:"Null"));
        return -1;
    }
    CConInfo& oInfo = e->second;

    // ��ȡ��Ϣ
    CMsgInfo oReqMsg;
    if(!oInfo.GetPop(oReqMsg, iMillSecTimeo))
    {
        char* pName = pHandle ? static_cast<char*>(pHandle) : "Null";
        return -2;
    }

    // ������Ϣģ������Ӧ����Ϣ
    bool bRet = false;
    CMsgInfo oRespMsg;
    if(oReqMsg.nMsgSrc == MSG_SRC_SEND)
    {
        CIFTSResp oIfts;
        bRet = oIfts.MakeRespPkg(oReqMsg, oRespMsg);
        if(!bRet)
        {
            LOGE("����Ӧ����Ϣʧ�ܣ���������[%s]", oReqMsg.psPkg);
        }
        // ��Ӧ���������ڴ����
        delete[] oReqMsg.psPkg;
        if(!bRet) return -3;
    }
    // ��ȡ�������ݰ�͸��
    else if(oReqMsg.nMsgSrc == MSG_SRC_FILE)
    {
        // �������з���İ�ת��
        // ����XML
        if(!oReqMsg.oPkg.Parse(oReqMsg.psPkg))
        {
            // ��Ӧ���������ڴ����
            delete[] oReqMsg.psPkg;
            return -5;
        }
        // ��ȡ��������Ϣ
        CMessgeHeader* pMsgHeader = dynamic_cast<CMessgeHeader*>(oReqMsg.oPkg.GetComponent("MsgHdr"));
        if(!pMsgHeader)
        {
            // ��Ӧ���������ڴ����
            delete[] oReqMsg.psPkg;
            return -6;
        }
        // �жϷ����Ƿ�Ϊ����
        if(pMsgHeader->oSender.strInstitutionType.at(0) != 'B')
        {
            // ��Ӧ���������ڴ����
            LOGD("���������ݰ����������ݴ�����������[%s]", oReqMsg.psPkg);
            delete[] oReqMsg.psPkg;
            return -2;
        }
        // ͸���������ݰ�
        oRespMsg = oReqMsg;
    }
    else
    {
        LOGE("�Ƿ�����Դ��������������������[%s]", oReqMsg.psPkg);
        delete[] oReqMsg.psPkg;
        return -4;
    }

    // ����Ӧ����Ϣ
    *ppsPkg = const_cast<char*>(oRespMsg.psPkg);
    *piOutPkgLen = oRespMsg.iPkgLen;
    *piErrSXCode = 0;
    LOGI("ģ��������Ϣ�ɹ�,����[%d] ����[%s]", oRespMsg.iPkgLen, *ppsPkg);
    
    return 0;
}

MRAPI_API int   CALL_METHOD MrIsLinkOK(void* pHandle)
{
    return (g_mapHandle.find(static_cast<char*>(pHandle)) != g_mapHandle.end());
}

MRAPI_API void    CALL_METHOD MrDestroy(void* pHandle)
{
    MrUninit(pHandle);
    SLogInst->Uninit();
}

MRAPI_API void  CALL_METHOD MrUninit(void* pHandle)
{
    char* pName = (char*)pHandle;
    map<string, CConInfo>::iterator e = g_mapHandle.find(pName);
    if(e != g_mapHandle.end())
    {
        LOGI("ɾ�����[%s]�ɹ�", static_cast<char*>(pHandle));
        g_mapHandle.erase(e);
        delete[] pName;
    }
    else if(pHandle)
    {
        LOGI("���[%s]�����ڣ�����ɾ��", static_cast<char*>(pHandle));
    }
    ExitProcess(0);
}

MRAPI_API void  CALL_METHOD ShowMsg()
{
    printf("##################\n\nHello,world\n\n\n#########\n");
}
