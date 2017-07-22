#pragma once
#include "stdafx.h"
#include "TcpParser.h"
#include "ShareLink.h"
#include<WinSock2.h>  
#include "Ws2tcpip.h"

class TcpClient
{
public:
    TcpClient();
    ~TcpClient();

    void Init(const char* ip, int port, size_t max_pack_size, TcpParser* parser = NULL);
    bool Connect(const char* ip, int port);
    bool Connect();
    bool IsConnected();
    size_t GetMsg(char* pBuf, size_t buff_size);
    void Send(void* pbuff, size_t size);
    void Stop();
    void Close();
    void CleanUp();
protected:
    virtual void DataArrial(int size);
private:
    static DWORD WINAPI ThreadFunc(LPVOID lpParameter);
    char m_ip[22];
    int m_port;
    SOCKET m_sock;
    volatile bool m_connected;
    HANDLE m_ThreadHandle;
    TcpParser* GetmsgParser;
    TcpParser* SendmsgParser;
private:
    ShareLink m_sendbufHead;
    ShareLink m_recvbufHead;
    size_t m_max_pack_size;
    CRITICAL_SECTION SendCS;
    volatile long m_stop; 
};

