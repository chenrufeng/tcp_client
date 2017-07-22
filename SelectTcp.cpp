// SelectTcp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Ws2tcpip.h"
#include<WinSock2.h>  
#include "TcpClient.h"
#include <fstream>
#include <time.h>
#include <stdio.h>
#include "ShareLink.h"
#pragma comment(lib,"ws2_32.lib")  
using namespace std;
int _tmain(int argc, _TCHAR* argv[]){
    TcpClient tc;
    int MAX_BUFF_SIZE = 10 * 1024;
    tc.Init("127.0.0.1", 8888, MAX_BUFF_SIZE);
    size_t total = 0;
    size_t elaspe = 0;
    time_t timeBegin;
    timeBegin = time(NULL);
    FILE *f;
    fopen_s(&f, "F:\\区域服务器.rar", "rb"); /*创建一个二进制文件只写*/
    long TT = 0;
    char* totalbuf = NULL;
    if (tc.Connect()){
        char* Pack = NULL;
        size_t msgLen = 0;
        while (true){
            //int nRnd = (rand() * rand()) % (MAX_BUFF_SIZE::MAX_SIZE) + 1;
            int nRnd = (MAX_BUFF_SIZE);
            Pack = new char[nRnd];
            int nlen = fread_s(Pack, nRnd, 1, nRnd, f);
            tc.Send(Pack, nlen);
            if (nlen <= 0)
            {
                printf("\n%d sent.\n", total);
                Sleep(1000 * 1000 * 1000);
                fclose(f); /*关闭文件*/
                tc.Stop();
                tc.Close();
                tc.Connect();

            }
            total += nlen;
            
            if (time(NULL) - timeBegin > (time_t)(elaspe + 2)){
                printf("\n%d sent.\n", total);
                elaspe = time(NULL) - timeBegin;
            }
            //delete[] Pack;
        }
        
    }
    return 0;
}

// int _tmain(int argc, _TCHAR* argv[]){
//     TcpClient tc;
//     tc.Init("127.0.0.1", 8888);
//     size_t total = 0;
//     size_t elaspe = 0;
//     time_t timeBegin;
//     timeBegin = time(NULL);
//     FILE *f;
//     fopen_s(&f, "e:\\b.rar", "wb"); /*创建一个二进制文件只写*/
//     long TT = 0;
//     char* totalbuf = NULL;
//     if (tc.Connect()){
//         char* Pack = new char[MAX_BUFF_SIZE::MAX_SIZE];
//         size_t msgLen = 0;
//         while (true){
//             msgLen = 0;
//             msgLen = tc.GetMsg(Pack, MAX_BUFF_SIZE::MAX_SIZE);
//             total += msgLen;
//             if (!tc.IsConnected()){
//                 printf("connection is break.");
//                 tc.Close();
//                 tc.Connect();
//                 
//                 
//                 fclose(f); /*关闭文件*/
//                 total = 0;
//                 TT = 0;
//             }
//             if (msgLen == 0){
//                 Sleep(1);
//             }
//             else
//             {
//                 fwrite(Pack, 1, msgLen, f);/*将6个浮点数写入文件中*/
//                 
//             }
//             if (time(NULL) - timeBegin > (time_t)(elaspe + 2)){
//                 printf("%d recieved.\n", total);
//                 elaspe = time(NULL) - timeBegin;
//             }
//         }
//         delete[] Pack;
//     }
//     return 0;
// }