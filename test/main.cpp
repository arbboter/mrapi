#include <iostream>
#include <Windows.h>
#include "../mrapi.h"
#pragma comment(lib,"mrapi.lib")

using namespace std;
int main()
{
    char* pName = "test";
    const char* pPwd = "123456";

    // 初始化
    STUConnInfo oConn = {"", 0, "", 0};
    MrInit(pName, "", NULL, oConn, NULL);

    int n=5;
    int nRet = 0;
    while(--n > 0)
    {
        for(int i=0; i<rand()%10; i++)
        {
            // 发送报文
            char* pReq = "<IFTS Len=\" 852\" DataVer =\"1.0.0.1\" SeqNo=\"           105000426\" Type=\"B\" Dup=\"N\" CheckSum=\"\"><MsgText><Trf.001.01><MsgHdr><Ver>1.0</Ver><SysType>0</SysType><InstrCd>12002</InstrCd><TradSrc>S</TradSrc><Sender><InstType>S</InstType><InstId>10241024</InstId><BrchId>0128</BrchId></Sender><Recver><InstType>B</InstType><InstId>4</InstId></Recver><Date>20200506</Date><Time>144934</Time><Ref><Ref>105000426</Ref></Ref></MsgHdr><Cust><Name>季俊峰</Name><CertId>440304199606242354</CertId><CertType>10</CertType></Cust><BkAcct><AcctSvcr><InstType></InstType><InstId>10241024</InstId><BrchId></BrchId></AcctSvcr><Id>6218567561358456</Id></BkAcct><ScAcct><AcctSvcr><InstType></InstType><InstId>10241024</InstId><BrchId>0105</BrchId></AcctSvcr><Id>10523265447</Id><Pwd><Pwd></Pwd></Pwd></ScAcct><Ccy>RMB</Ccy><TrfAmt>2000</TrfAmt></Trf.001.01></MsgText></IFTS>";
            char* pTmpBuf = new char[strlen(pReq)+1];
            strcpy(pTmpBuf, pReq);
            nRet = MrSend(pName, pTmpBuf, strlen(pTmpBuf), NULL, 2000);
            delete[] pTmpBuf;
            pTmpBuf = NULL;
            cout << "发送完成:" << nRet << std::endl;
        }

        // 接收报文
        char* pRecv = NULL;
        int nRecv = 0;
        int nExitCode = 0;
        STUMsgProperty oPty;
        while(!MrReceive3(pName, &pRecv, &nRecv, &nExitCode, &oPty, 2000))
        {
            MrReceive1_FreeBuf(pRecv);
            cout << "接收成功" << std::endl;
        }
        Sleep(1000);
    }

    return 0;
}