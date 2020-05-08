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

// 连接信息
class CConInfo
{
public:
    string              strAppId;
    string              strPasswd;
    RecallOnReceive     pOnReceive;
    STUConnInfo         oConnInfo;
    void*               pvUserData;

private:
    // 内部变量
    // 请求消息
    queue<CMsgInfo>     qReqMsg;
    // 线程锁
    CRITICAL_SECTION    csLock;
    // 文件修改时间
    map<string, FILETIME> mapFileTime;

    // 文件是否变了，看修改时间
    bool IsFileChange(const string& strFile)
    {
        // 获取文件信息
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

        // 查看文件是否更新
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
        // 初始化变量及临界变量
        pOnReceive = NULL;
        pvUserData = NULL;
        InitializeCriticalSection(&csLock);
    }
    ~CConInfo()
    {
        // 删除临界变量
        DeleteCriticalSection(&csLock);
    }

    // 锁处理
    void Lock()
    {
        EnterCriticalSection(&csLock);
    }
    void Unlock()
    {
        LeaveCriticalSection(&csLock);
    }

    // 从文件加载消息
    void LoadFile()
    {
        // 读取配置数据
        CIFTSResp oIfts;
        vector<string> vecXml;
        if(IsFileChange(oIfts.m_BankReqFile))
        {
            LOGD("读取银行请求记录数[%d]", CTxMsg::LoadMsg(oIfts.m_BankReqFile, vecXml))
        }
        if(IsFileChange(oIfts.m_BankAnsFile))
        {
            LOGD("读取银行应答记录数[%d]", CTxMsg::LoadMsg(oIfts.m_BankAnsFile, vecXml))
        }

        // 加载消息
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

    // 添加请求
    void Push(const CMsgInfo& oMsg)
    {
        Lock();
        qReqMsg.push(oMsg);
        Unlock();
    }
    // 取请求
    bool GetPop(CMsgInfo& oMsg, int iMillSecTimeo=2000)
    {
        bool bRet = false;
        // 无数据时超时等待
        while(iMillSecTimeo>0 && qReqMsg.empty())
        {
            iMillSecTimeo -= 100;
        }
        // 无数据尝试从文件加载数据
        if(qReqMsg.empty())
        {
            LoadFile();
        }
        // 尝试获取请求数据
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
map<string, CConInfo>        g_mapHandle;    // 连接信息

MRAPI_API void*  CALL_METHOD MrInit(const char* psAppID, const char* psPasswd,RecallOnReceive pOnReceive,const STUConnInfo oConnInfo, void* pvUserData)
{
    // 查看是否已存在
    char* pHandle = const_cast<char*>(psAppID);
    if(psAppID && g_mapHandle.find(pHandle) != g_mapHandle.end())
    {
        return pHandle;
    }
    // 生成句柄
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
    // 保存连接信息
    CConInfo oInfo;
    oInfo.strAppId = pHandle;
    oInfo.strPasswd = psPasswd;
    oInfo.pOnReceive = pOnReceive;
    oInfo.oConnInfo = oConnInfo;
    oInfo.pvUserData = pvUserData;

    // 保存到全局变量
    LOGI("初始化句柄[%s]成功", pHandle);
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

    // 查询句柄是否存在
    char* pName = (char*)pHandle;
    if(pHandle==NULL || g_mapHandle.find(pName) == g_mapHandle.end())
    {
        LOGE("句柄[%s]不存在", (pName ? pName:"Null"));
        return -1;
    }
    // 检查消息内容
    if(psPkg==NULL || iPkgLen<=0)
    {
        LOGE("请求内容非法，内容不允许为空");
        return -2;
    }
    // 获取消息内容
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
    LOGI("收到请求消息,长度[%d] 内容[%s]", iPkgLen, psPkg);

    // 检测顺序，为应答消息则直接丢掉
    if(CTxMsg::IsReqMsg(pSendBuf))
    {
        LOGD("应答消息，不处理已丢弃");
        return 0;
    }

    // 保存请求到本地
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

    // 查询句柄是否存在
    char* pName = (char*)pHandle;
    map<string, CConInfo>::iterator e = g_mapHandle.find(pName);
    if(pName==NULL || e==g_mapHandle.end())
    {
        LOGE("句柄[%s]不存在", (pName ? pName:"Null"));
        return -1;
    }
    CConInfo& oInfo = e->second;

    // 获取消息
    CMsgInfo oReqMsg;
    if(!oInfo.GetPop(oReqMsg, iMillSecTimeo))
    {
        char* pName = pHandle ? static_cast<char*>(pHandle) : "Null";
        return -2;
    }

    // 请求消息模拟生成应答消息
    bool bRet = false;
    CMsgInfo oRespMsg;
    if(oReqMsg.nMsgSrc == MSG_SRC_SEND)
    {
        CIFTSResp oIfts;
        bRet = oIfts.MakeRespPkg(oReqMsg, oRespMsg);
        if(!bRet)
        {
            LOGE("生成应答消息失败，请求内容[%s]", oReqMsg.psPkg);
        }
        // 对应请求函数的内存分配
        delete[] oReqMsg.psPkg;
        if(!bRet) return -3;
    }
    // 读取银行数据包透传
    else if(oReqMsg.nMsgSrc == MSG_SRC_FILE)
    {
        // 仅限银行发起的包转发
        // 解析XML
        if(!oReqMsg.oPkg.Parse(oReqMsg.psPkg))
        {
            // 对应请求函数的内存分配
            delete[] oReqMsg.psPkg;
            return -5;
        }
        // 获取发送者信息
        CMessgeHeader* pMsgHeader = dynamic_cast<CMessgeHeader*>(oReqMsg.oPkg.GetComponent("MsgHdr"));
        if(!pMsgHeader)
        {
            // 对应请求函数的内存分配
            delete[] oReqMsg.psPkg;
            return -6;
        }
        // 判断发送是否为银行
        if(pMsgHeader->oSender.strInstitutionType.at(0) != 'B')
        {
            // 对应请求函数的内存分配
            LOGD("非银行数据包，按无数据处理，请求内容[%s]", oReqMsg.psPkg);
            delete[] oReqMsg.psPkg;
            return -2;
        }
        // 透传银行数据包
        oRespMsg = oReqMsg;
    }
    else
    {
        LOGE("非法数据源包，丢弃处理，请求内容[%s]", oReqMsg.psPkg);
        delete[] oReqMsg.psPkg;
        return -4;
    }

    // 拷贝应答消息
    *ppsPkg = const_cast<char*>(oRespMsg.psPkg);
    *piOutPkgLen = oRespMsg.iPkgLen;
    *piErrSXCode = 0;
    LOGI("模拟银行消息成功,长度[%d] 内容[%s]", oRespMsg.iPkgLen, *ppsPkg);
    
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
        LOGI("删除句柄[%s]成功", static_cast<char*>(pHandle));
        g_mapHandle.erase(e);
        delete[] pName;
    }
    else if(pHandle)
    {
        LOGI("句柄[%s]不存在，无需删除", static_cast<char*>(pHandle));
    }
    ExitProcess(0);
}

MRAPI_API void  CALL_METHOD ShowMsg()
{
    printf("##################\n\nHello,world\n\n\n#########\n");
}
