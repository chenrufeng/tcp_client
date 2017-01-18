#pragma once
#include<WinSock2.h>  
#include "Ws2tcpip.h"
#include "TcpParser.h"
class TcpClient
{
public:
    enum MAX_BUFF_SIZE
    {
        MAX_SIZE = 256 * 1024,
    };
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
    struct Node
    {
        size_t   nStartIndex;
        size_t   nUsed;
        char cData[MAX_SIZE];
        Node* pNext;
        static volatile long Count; //test code
    public:
        Node();
        ~Node();
        void Reset(){
            nStartIndex = 0;
            pNext = NULL;
            nUsed = 0;
        }
        void Write(size_t size);
        void Write(const char* buf, size_t size);
        void Read(size_t size);
        void Read(char* pBuf, size_t size);
        void Peek(char* pBuf, size_t size);
        char* GetData();
        bool IsEmpty();
        bool IsFull();
        size_t Available();
        size_t GetUsed();
    };
    class ShareLink
    {
        Node* pHead;
        Node* pAppending;  
        size_t totalUsed;
    public:
        // if it doesn't have enough size to peek, then return false.
        bool Peek(char* pbuf, size_t size);
        size_t ReadMsg(char* pBuf, size_t& size, TcpParser* parser);
        // decrease count.
        void Read(size_t size);
        void Read(char* pBuf, size_t size);
        
        size_t GetUsed();
        bool IsEmpty();
        // if it doesn't have used node, then return null.
        Node* PopHeadNode();
        void  PushFreeNode(Node* node); 

        // Producers are responsible for allocation of pAppending.
        void Append(const char* pbuf, size_t size);
        // increase count.
        void Write(size_t size);
        // Appending thread must be a single thread.
        void GetAppendingPointer(char** ppBuf, size_t& size);
        ShareLink();
        ~ShareLink();
        CRITICAL_SECTION m_cs;
    };
    ShareLink m_sendbufHead;
    ShareLink m_recvbufHead;

    volatile long m_stop; 
};

