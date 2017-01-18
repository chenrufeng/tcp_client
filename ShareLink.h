#pragma once
#include "TcpParser.h"
#include<WinSock2.h>  
enum MAX_BUFF_SIZE
{
    MAX_SIZE = 256 * 1024,
};

struct Node
{
    size_t   nStartIndex;
    size_t   nUsed;
    char cData[MAX_SIZE];
    Node* pNext;
public:
    Node();
    ~Node();
    void Reset(){
        nStartIndex = 0;
        pNext = 0;
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