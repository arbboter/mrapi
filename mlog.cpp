#include "stdafx.h"
#include "mlog.h"
#include <sstream>
#include <time.h>
#include <stdarg.h>
#include <Windows.h>
#include <stdexcept>

using namespace std;
using namespace MR;


// 关闭特定编译警告
#pragma warning(disable:4996)


CSlog* CSlog::m_pInst = NULL;
char   g_pLogTag[][8] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL", "NULL"};


int CSlog::MakeMultiPath(const string& strPath)
{
    int nMakes = 0;
    string strCurPath;

    vector<string> vecPath;
    GetPathUnit(strPath, vecPath);

    for (size_t i=0; i<vecPath.size(); i++)
    {
        strCurPath += vecPath[i] + "/";
        if(!FileExist(strCurPath))
        {
            CreateDirectory(strCurPath.c_str(), NULL);
            nMakes += 1;
        }
    }

    return nMakes;
}

int CSlog::GetPathUnit(const string& strPath, vector<string>& vecPath)
{
    // 路径分为绝对路径和相对路径
    // 如:./log/,./../log,D:/log/20161031

    // 格式化路径，将\替换为/
    size_t nPos = 0;
    string strWinDim = "\\";
    string strFmtDim = "/";
    string strFmtPath = strPath;
    while((nPos=strFmtPath.find(strWinDim,nPos)) != string::npos)
    {
        strFmtPath.replace(nPos, strWinDim.length(), strFmtDim);
        nPos += strWinDim.length();
    }

    // 分割路径
    return Split(strFmtPath, strFmtDim, vecPath);
}

int CSlog::Split(const string& strSrc, const string& strDim, vector<string>& vecItems)
{
    size_t nPos = 0;
    size_t nLast = 0;
    while((nPos=strSrc.find(strDim,nPos)) != string::npos)
    {
        vecItems.push_back(strSrc.substr(nLast, nPos-nLast));
        nPos += strDim.length();
        nLast =  nPos;
    }
    // 处理尾巴
    size_t nLen = strSrc.size();
    if(nLast < nLen)
    {
        vecItems.push_back(strSrc.substr(nLast, nLen-nLast));
    }
    return vecItems.size();
}

void CSlog::SetCfg(const LV& lv, const string& strName, const string& strLogPath)
{
    if(!strLogPath.empty())
    {
        m_strLogPath = strLogPath;
        size_t nEnd = strLogPath.length()-1;
        if(strLogPath[nEnd] != '/' && strLogPath[nEnd]!='\\' )
        {
            m_strLogPath += "/";
        }
    }
    m_strAppName = strName;
    m_lvLog = lv;

    // 关闭文件
    Uninit();
}

CSlog* CSlog::Inst()
{
    // 非线程安全
    if(m_pInst == NULL)
    {
        m_pInst = new CSlog();
    }
    return m_pInst;
}

string CSlog::BuildInfo(const LogInfo& oInfo)
{
    stringstream ssLog;

    // [2016-10-31 15:16:52] [123,456] [main.cpp,68,GetUserName] #ERROR# This is a FATAL Log 998001
    ssLog << "[" << oInfo.strDateTime << "] "
          << "[" << oInfo.nPid << "," << oInfo.nTid << "] "
          << "[" << GetFileName(oInfo.strFile) << "," << oInfo.nLine << "," << oInfo.strFunc << "] "
          << "#" << oInfo.strTag << "# "
          << oInfo.strTitle
          << oInfo.strMsg << "\n";
    
    return ssLog.str();
}

string CSlog::BuildHeader(const string& strTag)
{
    stringstream ssLog;
    const char* pLineTag = "---------------------";

    // 日志头信息
    ssLog << pLineTag << strTag << "    " <<  __DATE__ << " " << __TIME__ << pLineTag << "\n";
    
    return ssLog.str();
}

std::string CSlog::GetDateTime(const DATE_FMT& fmt)
{
    time_t t = time(NULL); 
    char szBuf[64] = {0};
    if(fmt == DATE_FMT_LOG_SEC)
    {
        strftime(szBuf, sizeof(szBuf), "%Y%m%d %H:%M:%S",localtime(&t) ); 
    }
    else if(fmt == DATE_FMT_DATE)
    {
        strftime(szBuf, sizeof(szBuf), "%Y%m%d",localtime(&t) ); 
    }
    else if(fmt == DATE_FMT_LOG_MSEC)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        sprintf_s(szBuf, sizeof(szBuf), "%4d-%02d-%02d %02d:%02d:%02d.%03d",
                  st.wYear,st.wMonth,st.wDay, st.wHour,st.wMinute,st.wSecond, st.wMilliseconds);
    }
    return szBuf;
}


void CSlog::LogFormate(LogInfo& oInfo, const LV& lv, const int nLine, const char* pFunc, const char* pFile, const char* pFmt, ...)
{
    va_list args;

    oInfo.strDateTime = GetDateTime(DATE_FMT_LOG_MSEC);
    oInfo.nLogLv = lv;
    oInfo.nLine = nLine;
    oInfo.strFunc = pFunc;
    oInfo.strFile = pFile;
    oInfo.nPid = GetCurrentProcessId();
    oInfo.nTid = GetCurrentThreadId();
    oInfo.strTag = g_pLogTag[lv];

    va_start(args, pFmt);
    oInfo.strMsg = Formate(pFmt, args);
    va_end(args);
}

void MR::CSlog::LogBuf(LogInfo& oInfo, const LV& lv, const int nLine, const char* pFunc, const char* pFile, const char* pBuf, const int nLen)
{
    oInfo.strDateTime = GetDateTime(DATE_FMT_LOG_MSEC);
    oInfo.nLogLv = lv;
    oInfo.nLine = nLine;
    oInfo.strFunc = pFunc;
    oInfo.strFile = pFile;
    oInfo.nPid = GetCurrentProcessId();
    oInfo.nTid = GetCurrentThreadId();
    oInfo.strTag = g_pLogTag[lv];
    oInfo.strMsg.assign(pBuf, nLen);
}

size_t MR::CSlog::LogToFile(const int& nLogLvl, const string& strLog)
{
    FILE* fp = NULL;
    int nTryNum = 2;
    MR::CSlog::LV wlv = (MR::CSlog::LV)nLogLvl;
    while(--nTryNum > 0)
    {
        fp= ObtainFile(wlv);
        if(fp)
        {
            if(fwrite(strLog.c_str(), strLog.length(), 1, fp) != 1)
            {
                CloseFile(wlv);
            }
            else
            {
                fflush(fp);
                break;
            }
        }
    }

    return strLog.size();
}

unsigned int __stdcall MR::CSlog::LogWork(void* pLog)
{
    CSlog* pThis = (CSlog*)pLog;

    size_t nLogLvl = 0;
    queue<string>* pLogBuf = pThis->m_quLogBuf;
    string  strLog;
    bool    bFetchLog = false;
    int     nTryNum = 2;
    FILE*   fpLog = NULL;
    bool    bHaveLogs = false;

    while(pThis->m_bRun)
    {
        // 写日志到文件
        bHaveLogs = false;
        for (nLogLvl=0; nLogLvl<LV_MAX; nLogLvl++)
        {
            if(pLogBuf[nLogLvl].size() > 0)
            {
                // 获取队列日志
                bFetchLog = false;
                pThis->Lock(nLogLvl);
                if(pLogBuf[nLogLvl].size() > 0)
                {
                    strLog = pLogBuf[nLogLvl].front();
                    pLogBuf[nLogLvl].pop();
                    bFetchLog = true;
                }
                pThis->Unlock(nLogLvl);

                // 写日志到文件
                if(bFetchLog)
                {
                    bHaveLogs = true;
                    pThis->LogToFile(nLogLvl, strLog);
                }
            }
        }

        // 如果没有获取到日志，休眠一段时间
        if(!bHaveLogs)
        {
            // printf("没有日志了，睡一会儿\n");
            Sleep(1000);
        }
    }

    return 0;
}

std::string CSlog::Formate(const char * pFmt, ...)
{
    va_list args;
    string  strRet;

    va_start(args, pFmt);
    strRet = Formate(pFmt, args);
    va_end(args);

    return strRet;
}


std::string CSlog::Formate(const char * pFmt, va_list va)
{
    int     nLen = 0;
    string  strRet;

    nLen = _vscprintf(pFmt, va) + 1;

    char* pBuf = NULL;
    char* pNewBuf = NULL;
    try
    {
        static char pMiniBuf[1024] = {0};
        
        // 小内存不额外分配内存
        if(nLen < 1024)
        {
            pBuf = pMiniBuf;
        }
        else
        {
            pNewBuf = new char[nLen];
            if(pNewBuf == NULL)
            {
                // 内存分配失败
                throw runtime_error("alloc memory failed.");
            }
            pBuf = pNewBuf;
        }

        memset(pBuf, 0, sizeof(char)*nLen);
        vsprintf_s(pBuf, nLen, pFmt, va);
        strRet = pBuf;
    }
    catch(...)
    {
        // 异常错误
    }
    if(pNewBuf)
    {
        delete[] pBuf;
        pBuf = NULL;
    }

    return strRet;
}

size_t CSlog::WriteLogBuf(const LV& lv, const string& strLog)
{
    // return 0;
    if((lv<m_lvLog) || (lv>=LV_MAX))
    {
        return 0;
    }

    FILE* fp = NULL;
    LV wlv = lv;

    switch (m_fmLog)
    {
    case FM_ALL:
        {
            wlv = m_lvLog;
            break;
        }
    case FM_TREE:
    case FM_SELF:
        {
            wlv = lv;
            break;
        }
    }

    // 写当前日志
    Lock(wlv);
    m_quLogBuf[wlv].push(strLog);
    Unlock(wlv);
    
    // 写下级日志
    if(m_fmLog == FM_TREE)
    {
        WriteLogBuf((LV)(lv-1), strLog);
    }
    return strLog.length();
}

bool CSlog::Init()
{
    bool bRet = true;

    m_lvLog = LV_DEBUG;
    m_strLogPath = "./LOG/";
    m_strAppName = "MRAPI";
    m_nMaxFileSize = 1024*1024*20;
    m_nMaxFileNum = 20;

    for (int i=0; i<=LV_MAX; i++)
    {
        CloseFile((LV)i);
    }

    return bRet;
}

bool CSlog::Uninit()
{
    bool bRet = true;

    // 等待当前日志写完
    m_lvLog = LV_MAX;
    while(!m_quLogBuf->empty()) Sleep(100);
    // 关闭句柄文件
    for (int i=0; i<=LV_MAX; i++)
    {
        CloseFile((LV)i);
    }

    return bRet;
}

size_t CSlog::GetFileSize(const string& strFile)
{
    size_t nSize = 0;
#if 0
    struct _stat oInfo;
    _stat(strFile.c_str(), &oInfo);
    nSize = oInfo.st_size;
#else
    HANDLE hFile = CreateFile(strFile.c_str(), FILE_READ_EA, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        nSize = ::GetFileSize(hFile, NULL);
        CloseHandle(hFile);
    }
#endif
  
    return nSize;
}

bool CSlog::RollFile(const LV& lv, int nStart)
{
    bool bRet = true;
    string strPath;
    string strNewPath;

    for (int i=m_nMaxFileNum; i>0; i--)
    {
        strPath = m_strLogPath + GetName(lv, i-1);
        strNewPath = m_strLogPath + GetName(lv, i);
        
        // 删除旧文件
        if(FileExist(strNewPath.c_str()))
        {
            DeleteFile(strNewPath.c_str());
        }

        // 复制文件
        if(FileExist(strPath.c_str()))
        {
            bRet &= (MoveFile(strPath.c_str(), strNewPath.c_str()) == TRUE);
        }
    }
    DeleteFile(strPath.c_str());
    return bRet;
}

FILE* CSlog::ObtainFile(const LV& lv)
{
    string strName = m_strLogPath + GetName(lv, 0);

    int nTryNum = 2;

    do
    {
        nTryNum -= 1;
        if(m_fpLog[lv] == NULL)
        {
            m_fpLog[lv] = fopen(strName.c_str(), "a");
            if(m_fpLog[lv] == NULL)
            {
                // 打开文件失败，尝试创建目录
                MakeMultiPath(m_strLogPath);
            }
            else
            {
                break;
            }
        }
        else if(GetFileSize(strName.c_str()) >= m_nMaxFileSize)
        {
            CloseFile(lv);
            RollFile(lv);
            continue;
        }
        else
        {
            break;
        }
    }while(nTryNum>0);

    return m_fpLog[lv];
}

std::string CSlog::GetName(const LV& lv, int nIdx)
{
    string strRet = m_strAppName + "_" + GetDateTime(DATE_FMT_DATE) + "_" + g_pLogTag[lv];

    if(nIdx == 0)
    {
        strRet += ".log";
    }
    else
    {
        char szBuf[64] = {0};
        sprintf_s(szBuf, sizeof(szBuf), "_%02d.log", nIdx);
        strRet += szBuf;
    }
    return strRet;
}

int CSlog::CloseFile(const LV& lv)
{
    int nRet = 0;
    if(m_fpLog[lv] != NULL)
    {
        nRet = fclose(m_fpLog[lv]);
        m_fpLog[lv] = NULL;
    }
    return nRet;
}

bool CSlog::FileExist(const string& strFile)
{
    // 复制文件
    return (INVALID_FILE_ATTRIBUTES != GetFileAttributes(strFile.c_str()));
}

void CSlog::SetFileMode(const FILE_MODE& fm)
{
    m_fmLog = fm;
}

void CSlog::SetFileRoll(const size_t& nMaxFileSize, const int& nMaxFileNum)
{
    m_nMaxFileSize = nMaxFileSize;
    m_nMaxFileNum = nMaxFileNum;
}

std::string CSlog::GetFileName(const string& strPath)
{
    string  strName;
    vector<string> vecPath;

    GetPathUnit(strPath, vecPath);
    if(vecPath.size() > 0)
    {
        strName = vecPath[vecPath.size()-1];
    }
    return strName;
}

void CSlog::Lock(int nLv)
{
    if(m_bEnableTS)
    {
        EnterCriticalSection(&m_csLogQueue[nLv]);
    }
}

void CSlog::Unlock(int nLv)
{
    if(m_bEnableTS)
    {
        LeaveCriticalSection(&m_csLogQueue[nLv]);
    }
}




/** Dump相关函数 **/

int MR::GenerateMiniDump(HANDLE hFile, PEXCEPTION_POINTERS pExceptionPointers, PCHAR pwAppName)
{
    BOOL bOwnDumpFile = FALSE;
    HANDLE hDumpFile = hFile;
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;

    typedef BOOL(WINAPI * MiniDumpWriteDumpT)(
        HANDLE,
        DWORD,
        HANDLE,
        MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION,
        PMINIDUMP_USER_STREAM_INFORMATION,
        PMINIDUMP_CALLBACK_INFORMATION
        );

    MiniDumpWriteDumpT pfnMiniDumpWriteDump = NULL;
    HMODULE hDbgHelp = LoadLibrary("DbgHelp.dll");
    if (hDbgHelp)
    {
        pfnMiniDumpWriteDump = (MiniDumpWriteDumpT)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
    }

    if (pfnMiniDumpWriteDump)
    {
        if (hDumpFile == NULL || hDumpFile == INVALID_HANDLE_VALUE)
        {
            TCHAR szFileName[MAX_PATH] = { 0 };
            TCHAR* szVersion = "jtyhsfcg";
            SYSTEMTIME stLocalTime;

            GetLocalTime(&stLocalTime);

            CreateDirectory(szFileName, NULL);

            sprintf(szFileName, "%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
                szVersion,
                stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
                GetCurrentProcessId(), GetCurrentThreadId());
            hDumpFile = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

            bOwnDumpFile = TRUE;
            OutputDebugString(szFileName);
        }

        if (hDumpFile != INVALID_HANDLE_VALUE)
        {
            ExpParam.ThreadId = GetCurrentThreadId();
            ExpParam.ExceptionPointers = pExceptionPointers;
            ExpParam.ClientPointers = FALSE;

            pfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                hDumpFile, MiniDumpWithDataSegs, (pExceptionPointers ? &ExpParam : NULL), NULL, NULL);

            if (bOwnDumpFile)
            {
                CloseHandle(hDumpFile);
            }
        }
    }

    if (hDbgHelp != NULL)
    {
        FreeLibrary(hDbgHelp);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


LONG WINAPI MR::ExceptionFilter(LPEXCEPTION_POINTERS lpExceptionInfo)
{
    if (IsDebuggerPresent())
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    return MR::GenerateMiniDump(NULL, lpExceptionInfo, "mrapi.log");
}