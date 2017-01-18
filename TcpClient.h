#pragma once
#include<WinSock2.h>  
#include "Ws2tcpip.h"
#include "TcpParser.h"
#include "ShareLink.h"
class TcpClient
{
public:
    TcpClient();
    ~TcpClient();

    void Init(const char* ip, int port);
    bool Connect();
    bool IsConnected();
    size_t GetMsg(char* pBuf, size_t buff_size);
    void Send(void* pbuff, size_t size);
    void Stop();
    void Close();
    void CleanUp();
private:
    static DWORD WINAPI ThreadFunc(LPVOID lpParameter);
    char m_ip[22];
    int m_port;
    SOCKET m_sock;
    volatile bool m_connected;
    HANDLE m_ThreadHandle;
    TcpParser m_parser;
private:
    ShareLink m_sendbufHead;
    ShareLink m_recvbufHead;

    volatile long m_stop; 
};

