// SelectTcp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Ws2tcpip.h"
#include<WinSock2.h>  
#include "TcpClient.h"
#include <time.h>
#pragma comment(lib,"ws2_32.lib")  
using namespace std;
int _tmain(int argc, _TCHAR* argv[]){
    TcpClient tc;
    tc.Init("127.0.0.1", 8888);
    size_t total = 0;
    int elaspe = 0;
    time_t timeBegin;
    timeBegin = time(NULL);
    if (tc.Connect()){
        char* Pack = new char[TcpClient::MAX_BUFF_SIZE::MAX_SIZE];
        int msgLen = 0;
        while (true){
            msgLen = tc.GetMsg(Pack, TcpClient::MAX_BUFF_SIZE::MAX_SIZE);
            total += msgLen;
            if (!tc.IsConnected()){
                printf("connection is break.");
                tc.Close();
                tc.Connect();
                total = 0;
            }
            if (msgLen == 0){
                Sleep(1);
            }
            if (time(NULL) - timeBegin > elaspe + 2){
                printf("%d recieved.\n", total);
                elaspe = time(NULL) - timeBegin;
            }
        }
        delete[] Pack;
    }
    return 0;
}