#ifndef __SLOG_H_20161031__
#define __SLOG_H_20161031__

#include <string>
#include <vector>
#include <queue>
#include <stdarg.h>
#include <Windows.h>
#include <DbgHelp.h>
#include <process.h>

using namespace std;
namespace MR{


// 程序崩溃Dump文件
int  GenerateMiniDump(HANDLE hFile, PEXCEPTION_POINTERS pExceptionPointers, PCHAR pwAppName);
LONG WINAPI ExceptionFilter(LPEXCEPTION_POINTERS lpExceptionInfo);

class CSlog
{
public:
    enum LV
    {
        LV_DEBUG    = 0,
        LV_INFO     = 1,
        LV_WARN     = 2,
        LV_ERROR    = 3,
        LV_FATAL    = 4,

        LV_MAX,
    };

    enum FILE_MODE
    {
        FM_SELF,        // 日志文件仅存储自身等级的日志
        FM_TREE,        // 日志文件存储自身等级及以上的日志
        FM_ALL,         // 所以日志都存储到当前设定的日志等级文件中
    };

    enum DATE_FMT
    {
        DATE_FMT_LOG_MSEC,
        DATE_FMT_LOG_SEC,
        DATE_FMT_DATE,
    };
    
    // 日志信息
    typedef struct _log_info_
    {
        unsigned int    nTid;           // 线程ID
        unsigned int    nPid;           // 进程ID
        unsigned int    nLine;          // 行号
        int             nLogLv;         // 日志登记
        string          strFile;        // 文件名
        string          strFunc;        // 函数名
        string          strTag;         // 日志等级标签名
        string          strMsg;         // 日志信息
        string          strDateTime;    // 日志时间
        string          strTitle;       // 标题（抬头）
    }LogInfo;

public:
    // 从路径获取文件名
    string GetFileName(const string& strPath);
    // 文件是否存在
    bool FileExist(const string& strFile);
    // 获取文件大小
    size_t GetFileSize(const string& strFile);
    // 创建多级目录
    int MakeMultiPath(const string& strPath);
    // 获取路径单元，分割路径
    int GetPathUnit(const string& strPath, vector<string>& vecPath);
    // 格式化字符串到string
    static string Formate(const char * pFmt, ...);
    static string Formate(const char * pFmt, va_list va);

    // 字符串分割 
    static int Split(const string& strSrc, const string& strDim, vector<string>& vecItems);
    // 获取当前时间
    static string GetDateTime(const DATE_FMT& fmt);

public:
    // 对外提供的参数调整设置
    void    SetCfg(const LV& lv, const string& strName, const string& strPath="");
    void    SetFileMode(const FILE_MODE& fm);
    void    SetFileRoll(const size_t& nMaxFileSize, const int& nMaxFileNum);
    static  CSlog*  Inst();

    // 初始化和逆初始化函数
    bool Init();
    bool Uninit();

public:
    // 生成日志信息
    void    LogFormate(LogInfo& oInfo, const LV& lv, const int nLine, const char* pFunc, const char* pFile, const char* pFmt, ...);
    void    LogBuf(LogInfo& oInfo, const LV& lv, const int nLine, const char* pFunc, const char* pFile, const char* pBuf, const int nLen);
    string  BuildInfo(const LogInfo& oInfo);
    string  BuildHeader(const string& strTag);
    // 写日志
    size_t  WriteLogBuf(const LV& lv, const string& strLog);

protected:
    // 日志滚动
    bool    RollFile(const LV& lv, int nStart=0);
    // 获取写入文件
    FILE*   ObtainFile(const LV& lv);
    // 获取日志文件名
    string GetName(const LV& lv, int nIdx);
    // 写日志到文件
    size_t LogToFile(const int& nLogLvl, const string& strLog);
    int    CloseFile(const LV& lv);
    void   Lock(int nLv);
    void   Unlock(int nLv);

protected:
    // 日志线程函数，常驻任务处理，如：
    // 1.缓存队列数据写入文件
    static unsigned int __stdcall LogWork(void* pLog);

private:
    CSlog()
    {
        for (int i=0; i<=LV_MAX; i++)
        {
            m_fpLog[i] = NULL;
            InitializeCriticalSection(&m_csLogQueue[i]);
        }

        Init();
        m_fmLog = FM_ALL;

        // 创建日志线程
        m_hWorkThread = (HANDLE)_beginthreadex(NULL, 0, LogWork, this, CREATE_SUSPENDED, NULL);
        if(m_hWorkThread == INVALID_HANDLE_VALUE)
        {
            throw "failed to create log work thread.";
        }

        m_bEnableTS = true;
        m_bRun = true;

        // 启动日志线程
        ResumeThread(m_hWorkThread);
    }

    ~CSlog(void)
    {
        m_bRun = false;
        Uninit();

        for (int i=0; i<=LV_MAX; i++)
        {
            m_fpLog[i] = NULL;
            DeleteCriticalSection(&m_csLogQueue[i]);
        }
        WaitForSingleObject(m_hWorkThread, INFINITE);
    }

private:
    static CSlog*  m_pInst;     // 日志句柄

protected:
    CRITICAL_SECTION    m_csLogQueue[LV_MAX+1];     // 日志队列锁
    FILE*               m_fpLog[LV_MAX+1];          // 日志文件句柄
    queue<string>       m_quLogBuf[LV_MAX+1];       // 日志队列缓冲区
    LV                  m_lvLog;                    // 日志等级
    string              m_strLogPath;               // 日志文件路径
    string              m_strAppName;               // 日志程序名称，日志文件名中包含该字段
    size_t              m_nMaxFileSize;             // 单个日志文件最大值
    int                 m_nMaxFileNum;              // 单级日志文件最大数
    FILE_MODE           m_fmLog;                    // 日志文件模式
    bool                m_bEnableTS;                // 是否启用线程安全
    HANDLE              m_hWorkThread;              // 工作线程句柄
    bool                m_bRun;                     // 是否运行
};

#define SLogInst        (CSlog::Inst())


#define LOGLv(lv,fmt, ...)  do{\
                                CSlog::LogInfo __LOGLv_lg__;\
                                string __LOGLv_strLog__;\
                                SLogInst->LogFormate(__LOGLv_lg__, lv, __LINE__, __FUNCTION__, __FILE__, fmt, ##__VA_ARGS__);\
                                __LOGLv_strLog__ = SLogInst->BuildInfo(__LOGLv_lg__);\
                                SLogInst->WriteLogBuf(lv, __LOGLv_strLog__);\
                            }while(0);

#define BLOGLv(lv, pTitle, pBuf, nLen)  do{\
                                    CSlog::LogInfo __LOGLv_lg__;\
                                    string __LOGLv_strLog__;\
                                    SLogInst->LogBuf(__LOGLv_lg__, lv, __LINE__, __FUNCTION__, __FILE__, pBuf, nLen); \
                                    __LOGLv_lg__.strTitle = pTitle; \
                                    __LOGLv_strLog__ =  SLogInst->BuildInfo(__LOGLv_lg__);\
                                    SLogInst->WriteLogBuf(lv, __LOGLv_strLog__);\
                                }while(0);

#define LOGLvH(lv,tag)      do{\
                                string __LOGLv_strLog__;\
                                __LOGLv_strLog__ = SLogInst->BuildHeader(tag);\
                                SLogInst->WriteLogBuf(lv, __LOGLv_strLog__);\
                             }while(0);

// 格式化日志接口
#define LOGD(fmt,...)   LOGLv(CSlog::LV_DEBUG,fmt,  ##__VA_ARGS__)
#define LOGI(fmt,...)   LOGLv(CSlog::LV_INFO ,fmt,  ##__VA_ARGS__)
#define LOGW(fmt,...)   LOGLv(CSlog::LV_WARN ,fmt,  ##__VA_ARGS__)
#define LOGE(fmt,...)   LOGLv(CSlog::LV_ERROR,fmt,  ##__VA_ARGS__)
#define LOGF(fmt,...)   LOGLv(CSlog::LV_FATAL,fmt,  ##__VA_ARGS__)

// 二进制安全日期
#define BLOGD(pTitle, pBuf, nLen)   BLOGLv(CSlog::LV_DEBUG, pTitle, pBuf, nLen)
#define BLOGI(pTitle, pBuf, nLen)   BLOGLv(CSlog::LV_INFO , pTitle, pBuf, nLen)
#define BLOGW(pTitle, pBuf, nLen)   BLOGLv(CSlog::LV_WARN , pTitle, pBuf, nLen)
#define BLOGE(pTitle, pBuf, nLen)   BLOGLv(CSlog::LV_ERROR, pTitle, pBuf, nLen)
#define BLOGF(pTitle, pBuf, nLen)   BLOGLv(CSlog::LV_FATAL, pTitle, pBuf, nLen)

// 日志分段标签，默认标签当前函数名
#define LOGDH()   LOGLvH(CSlog::LV_DEBUG,__FUNCTION__)
#define LOGIH()   LOGLvH(CSlog::LV_INFO ,__FUNCTION__)
#define LOGWH()   LOGLvH(CSlog::LV_WARN ,__FUNCTION__)
#define LOGEH()   LOGLvH(CSlog::LV_ERROR,__FUNCTION__)
#define LOGFH()   LOGLvH(CSlog::LV_FATAL,__FUNCTION__)

// 日志分段标签
#define LOGDHT(tag)   LOGLvH(CSlog::LV_DEBUG,tag)
#define LOGIHT(tag)   LOGLvH(CSlog::LV_INFO ,tag)
#define LOGWHT(tag)   LOGLvH(CSlog::LV_WARN ,tag)
#define LOGEHT(tag)   LOGLvH(CSlog::LV_ERROR,tag)
#define LOGFHT(tag)   LOGLvH(CSlog::LV_FATAL,tag)

}

#endif
