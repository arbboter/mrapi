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


// �������Dump�ļ�
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
        FM_SELF,        // ��־�ļ����洢����ȼ�����־
        FM_TREE,        // ��־�ļ��洢����ȼ������ϵ���־
        FM_ALL,         // ������־���洢����ǰ�趨����־�ȼ��ļ���
    };

    enum DATE_FMT
    {
        DATE_FMT_LOG_MSEC,
        DATE_FMT_LOG_SEC,
        DATE_FMT_DATE,
    };
    
    // ��־��Ϣ
    typedef struct _log_info_
    {
        unsigned int    nTid;           // �߳�ID
        unsigned int    nPid;           // ����ID
        unsigned int    nLine;          // �к�
        int             nLogLv;         // ��־�Ǽ�
        string          strFile;        // �ļ���
        string          strFunc;        // ������
        string          strTag;         // ��־�ȼ���ǩ��
        string          strMsg;         // ��־��Ϣ
        string          strDateTime;    // ��־ʱ��
        string          strTitle;       // ���⣨̧ͷ��
    }LogInfo;

public:
    // ��·����ȡ�ļ���
    string GetFileName(const string& strPath);
    // �ļ��Ƿ����
    bool FileExist(const string& strFile);
    // ��ȡ�ļ���С
    size_t GetFileSize(const string& strFile);
    // �����༶Ŀ¼
    int MakeMultiPath(const string& strPath);
    // ��ȡ·����Ԫ���ָ�·��
    int GetPathUnit(const string& strPath, vector<string>& vecPath);
    // ��ʽ���ַ�����string
    static string Formate(const char * pFmt, ...);
    static string Formate(const char * pFmt, va_list va);

    // �ַ����ָ� 
    static int Split(const string& strSrc, const string& strDim, vector<string>& vecItems);
    // ��ȡ��ǰʱ��
    static string GetDateTime(const DATE_FMT& fmt);

public:
    // �����ṩ�Ĳ�����������
    void    SetCfg(const LV& lv, const string& strName, const string& strPath="");
    void    SetFileMode(const FILE_MODE& fm);
    void    SetFileRoll(const size_t& nMaxFileSize, const int& nMaxFileNum);
    static  CSlog*  Inst();

    // ��ʼ�������ʼ������
    bool Init();
    bool Uninit();

public:
    // ������־��Ϣ
    void    LogFormate(LogInfo& oInfo, const LV& lv, const int nLine, const char* pFunc, const char* pFile, const char* pFmt, ...);
    void    LogBuf(LogInfo& oInfo, const LV& lv, const int nLine, const char* pFunc, const char* pFile, const char* pBuf, const int nLen);
    string  BuildInfo(const LogInfo& oInfo);
    string  BuildHeader(const string& strTag);
    // д��־
    size_t  WriteLogBuf(const LV& lv, const string& strLog);

protected:
    // ��־����
    bool    RollFile(const LV& lv, int nStart=0);
    // ��ȡд���ļ�
    FILE*   ObtainFile(const LV& lv);
    // ��ȡ��־�ļ���
    string GetName(const LV& lv, int nIdx);
    // д��־���ļ�
    size_t LogToFile(const int& nLogLvl, const string& strLog);
    int    CloseFile(const LV& lv);
    void   Lock(int nLv);
    void   Unlock(int nLv);

protected:
    // ��־�̺߳�������פ�������磺
    // 1.�����������д���ļ�
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

        // ������־�߳�
        m_hWorkThread = (HANDLE)_beginthreadex(NULL, 0, LogWork, this, CREATE_SUSPENDED, NULL);
        if(m_hWorkThread == INVALID_HANDLE_VALUE)
        {
            throw "failed to create log work thread.";
        }

        m_bEnableTS = true;
        m_bRun = true;

        // ������־�߳�
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
    static CSlog*  m_pInst;     // ��־���

protected:
    CRITICAL_SECTION    m_csLogQueue[LV_MAX+1];     // ��־������
    FILE*               m_fpLog[LV_MAX+1];          // ��־�ļ����
    queue<string>       m_quLogBuf[LV_MAX+1];       // ��־���л�����
    LV                  m_lvLog;                    // ��־�ȼ�
    string              m_strLogPath;               // ��־�ļ�·��
    string              m_strAppName;               // ��־�������ƣ���־�ļ����а������ֶ�
    size_t              m_nMaxFileSize;             // ������־�ļ����ֵ
    int                 m_nMaxFileNum;              // ������־�ļ������
    FILE_MODE           m_fmLog;                    // ��־�ļ�ģʽ
    bool                m_bEnableTS;                // �Ƿ������̰߳�ȫ
    HANDLE              m_hWorkThread;              // �����߳̾��
    bool                m_bRun;                     // �Ƿ�����
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

// ��ʽ����־�ӿ�
#define LOGD(fmt,...)   LOGLv(CSlog::LV_DEBUG,fmt,  ##__VA_ARGS__)
#define LOGI(fmt,...)   LOGLv(CSlog::LV_INFO ,fmt,  ##__VA_ARGS__)
#define LOGW(fmt,...)   LOGLv(CSlog::LV_WARN ,fmt,  ##__VA_ARGS__)
#define LOGE(fmt,...)   LOGLv(CSlog::LV_ERROR,fmt,  ##__VA_ARGS__)
#define LOGF(fmt,...)   LOGLv(CSlog::LV_FATAL,fmt,  ##__VA_ARGS__)

// �����ư�ȫ����
#define BLOGD(pTitle, pBuf, nLen)   BLOGLv(CSlog::LV_DEBUG, pTitle, pBuf, nLen)
#define BLOGI(pTitle, pBuf, nLen)   BLOGLv(CSlog::LV_INFO , pTitle, pBuf, nLen)
#define BLOGW(pTitle, pBuf, nLen)   BLOGLv(CSlog::LV_WARN , pTitle, pBuf, nLen)
#define BLOGE(pTitle, pBuf, nLen)   BLOGLv(CSlog::LV_ERROR, pTitle, pBuf, nLen)
#define BLOGF(pTitle, pBuf, nLen)   BLOGLv(CSlog::LV_FATAL, pTitle, pBuf, nLen)

// ��־�ֶα�ǩ��Ĭ�ϱ�ǩ��ǰ������
#define LOGDH()   LOGLvH(CSlog::LV_DEBUG,__FUNCTION__)
#define LOGIH()   LOGLvH(CSlog::LV_INFO ,__FUNCTION__)
#define LOGWH()   LOGLvH(CSlog::LV_WARN ,__FUNCTION__)
#define LOGEH()   LOGLvH(CSlog::LV_ERROR,__FUNCTION__)
#define LOGFH()   LOGLvH(CSlog::LV_FATAL,__FUNCTION__)

// ��־�ֶα�ǩ
#define LOGDHT(tag)   LOGLvH(CSlog::LV_DEBUG,tag)
#define LOGIHT(tag)   LOGLvH(CSlog::LV_INFO ,tag)
#define LOGWHT(tag)   LOGLvH(CSlog::LV_WARN ,tag)
#define LOGEHT(tag)   LOGLvH(CSlog::LV_ERROR,tag)
#define LOGFHT(tag)   LOGLvH(CSlog::LV_FATAL,tag)

}

#endif
