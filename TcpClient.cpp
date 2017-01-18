#include "stdafx.h"
#include "TcpClient.h"
#include "WinBase.h"
#include "assert.h"
#pragma comment(lib,"ws2_32.lib")  

TcpClient::TcpClient()
{
}
TcpClient::~TcpClient()
{
}
void TcpClient::Init(const char* ip, int port)
{
    //网络初始化  
    WSADATA wsaData;
    WSAStartup(MAKEWORD(1, 1), &wsaData);
    memcpy(this->m_ip, ip, sizeof(this->m_ip));
    this->m_port = port;
    this->m_connected = false;
    this->m_ThreadHandle = NULL;
    this->m_sock = INVALID_SOCKET;
    this->m_stop = 0;
}
bool TcpClient::Connect()
{
    this->m_sock = socket(AF_INET, SOCK_STREAM, 0);

    SOCKADDR_IN addrSrv;
    ZeroMemory(&addrSrv, sizeof(addrSrv));
    addrSrv.sin_family = AF_INET;
    inet_pton(AF_INET, this->m_ip, (void *)&addrSrv.sin_addr);
    addrSrv.sin_port = htons(this->m_port);
    int nRet = connect(this->m_sock, reinterpret_cast<SOCKADDR*>(&addrSrv), sizeof(SOCKADDR));
    this->m_connected = (nRet == 0);
    if (this->m_connected)
    {
        //设置为非阻塞模式  
        unsigned long nonBlock = 1;
        ioctlsocket(this->m_sock, FIONBIO, &nonBlock);
        InterlockedExchange(&this->m_stop, 0);
        this->m_ThreadHandle = CreateThread(NULL, 0, ThreadFunc, (LPVOID)this, 0, 0);
    }
    return this->m_connected;
}
void TcpClient::Close(){
    if (this->m_ThreadHandle)
    {
        WaitForSingleObject(this->m_ThreadHandle, INFINITE);
        ::CloseHandle(this->m_ThreadHandle);
        this->m_ThreadHandle = NULL;
    }
    if (this->m_sock != INVALID_SOCKET)
    {
        closesocket(this->m_sock);
        this->m_sock = INVALID_SOCKET;
    }
}
bool TcpClient::IsConnected()
{
    return this->m_connected;
}
size_t TcpClient::GetMsg(char* pBuf, size_t buff_size)
{
    return m_recvbufHead.ReadMsg(pBuf, buff_size, &m_parser);
}
void TcpClient::Send(void* pbuff, size_t size)
{
    if (size < 1) return;
    assert(size <= MAX_BUFF_SIZE::MAX_SIZE);
    this->m_sendbufHead.Append((const char *)pbuff, size);
}
void TcpClient::Stop()
{
    InterlockedIncrement(&this->m_stop);
}
void TcpClient::CleanUp()
{
    WSACleanup();
}
DWORD WINAPI TcpClient::ThreadFunc(LPVOID lpParameter){
    TcpClient* mine = (TcpClient*)lpParameter;
    FD_SET writeSet;
    FD_SET readSet;
    timeval timeout = { 1, 0 };
    int total;
    Node* sending_buf = NULL;
    struct Recving_Pointer
    {
        size_t size;
        size_t nUsed;
        char* pointer;
        Recving_Pointer(){
            Reset();
        }
        void Reset(){
            size = 0;
            nUsed = 0;
            pointer = NULL;
        }
        bool IsFull(){
            return size <= 0;
        }
        void Received(size_t n){
            nUsed += n;
        }
        char* GetPointer(){
            return pointer;
        }
        size_t GetSize(){
            return size;
        }
        size_t GetUsed(){
            return nUsed;
        }
        void   FetchUsed(){
            pointer += nUsed;
            size -= nUsed;
            nUsed = 0;            
        }
    }recving_pointer;
    while (mine->m_stop == 0)
    {
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        FD_SET(mine->m_sock, &readSet);
        FD_SET(mine->m_sock, &writeSet);

        if (sending_buf == NULL){
            sending_buf = mine->m_sendbufHead.PopHeadNode();
        }
        if (recving_pointer.pointer == NULL){
            mine->m_recvbufHead.GetAppendingPointer(&recving_pointer.pointer, recving_pointer.size);
        }
        
        if (sending_buf && sending_buf->GetUsed() > 0)
        {
            total = select(0, &readSet, &writeSet, nullptr, &timeout);
        }
        else
        {
            total = select(0, &readSet, nullptr, nullptr, &timeout);
        }

        if (total == 0) // timeout
        {
        }
        else
        {
#pragma region socket receive
            if (FD_ISSET(mine->m_sock, &readSet))
            {
                //1.If listen has been called and a connection is pending, accept will succeed.
                //2.Data is available for reading(includes OOB data if SO_OOBINLINE is enabled).
                //3.Connection has been closed / reset / terminated.
                size_t recvn = 0;
                recvn = recv(mine->m_sock, recving_pointer.GetPointer(), recving_pointer.GetSize(), 0);
                //if the connection has been gracefully closed, the return value is zero.
                if (recvn == 0) {
                    printf("recv failed: %d\n", WSAGetLastError());
                    break;
                }
                if (recvn == SOCKET_ERROR) {
                    printf("recv failed: %d\n", WSAGetLastError());
                    break;
                }
                recving_pointer.Received(recvn);
                mine->m_recvbufHead.Write(recving_pointer.GetUsed());
                recving_pointer.FetchUsed();

                if (recving_pointer.IsFull())
                {
                    recving_pointer.Reset();
                }
            }
#pragma endregion
#pragma region socket send
            if (FD_ISSET(mine->m_sock, &writeSet))
            {
                //1.If processing a connect call(nonblocking), connection has succeeded.
                //2.Data can be sent.
                if (sending_buf && sending_buf->GetUsed() > 0)
                {
                    size_t bytes_sent = send(mine->m_sock, (const char*)sending_buf->GetData(), sending_buf->GetUsed(), 0);
                    if (bytes_sent == SOCKET_ERROR) {
                        size_t ret_err = WSAGetLastError();
                        if (WSAEWOULDBLOCK != ret_err)
                        {
                            printf("send failed: %d\n", ret_err);
                            break;
                        }
                    }
                    sending_buf->Read(bytes_sent);
                    if (sending_buf->IsEmpty())
                    {
                        mine->m_sendbufHead.PushFreeNode(sending_buf);
                        sending_buf = NULL;
                    }
                }
            }
#pragma endregion
        }
    }
    mine->m_connected = false;
    return 0;
}
volatile long TcpClient::Node::Count = 0;
TcpClient::Node::Node(){
    ::InterlockedIncrement(&TcpClient::Node::Count);
    Reset();
}
TcpClient::Node::~Node(){
    ::InterlockedDecrement(&TcpClient::Node::Count);
}
void TcpClient::Node::Write(size_t size){
    nUsed += size;
}
void TcpClient::Node::Write(const char* buf, size_t size){    
    memcpy(cData + nStartIndex + nUsed, buf, size);
    nUsed += size;
}
void TcpClient::Node::Read(size_t size){
    nUsed -= size;
    nStartIndex += size;
}
void TcpClient::Node::Read(char* pBuf, size_t size){
    memcpy(pBuf, cData + nStartIndex, size);
    nUsed -= size;
    nStartIndex += size;
}
void TcpClient::Node::Peek(char* pBuf, size_t size){
    memcpy(pBuf, cData + nStartIndex, size);
}
bool TcpClient::Node::IsEmpty(){
    return (this->nUsed <= 0);
}
bool TcpClient::Node::IsFull(){
    return Available() == 0;
}
char* TcpClient::Node::GetData(){
    return cData + nStartIndex;
}
size_t TcpClient::Node::Available(){
    return MAX_SIZE - nStartIndex - nUsed;
}
size_t TcpClient::Node::GetUsed(){
    return this->nUsed;
}
TcpClient::ShareLink::ShareLink(){
    pHead = NULL;
    pAppending = NULL;
    totalUsed = 0;
    ::InitializeCriticalSection(&this->m_cs);
}
TcpClient::ShareLink::~ShareLink(){
    while (pHead)
    {
        Node* t = pHead->pNext;
        delete pHead;
        pHead = t;
    }
    ::DeleteCriticalSection(&this->m_cs);
}
bool TcpClient::ShareLink::Peek(char* pbuf, size_t size){
    bool ok = false;
    ::EnterCriticalSection(&this->m_cs);
    if (this->GetUsed() >= size){
        if (pbuf && size > 0)
        {
            if (pHead->GetUsed() > size){
                pHead->Peek(pbuf, size);
            }
            else{
                size_t rest = size - pHead->GetUsed();
                pHead->Peek(pbuf, pHead->GetUsed());
                pHead->pNext->Peek(pbuf + pHead->GetUsed(), rest);
            }
        }
        ok = true;
    }
    ::LeaveCriticalSection(&this->m_cs);
    return ok;
}
size_t TcpClient::ShareLink::ReadMsg(char* pBuf, size_t& size, TcpParser* parser){
    int nRead = 0;
    ::EnterCriticalSection(&this->m_cs);
    if (this->Peek(parser->GetHeader(), parser->GetHeaderSize()))
    {
        if (parser->GetBodySize() > 0)
        {
            if (this->GetUsed() > parser->GetHeaderSize() + parser->GetBodySize())
            {
                nRead = parser->GetBodySize();
                this->Read(parser->GetHeaderSize());
                this->Read(pBuf, nRead);
            }
        }
        else
        {// no separator
            nRead = min(size, this->GetUsed() - parser->GetHeaderSize());
            if (nRead > 0)
            {
                this->Read(parser->GetHeaderSize());
                this->Read(pBuf, nRead);
            }
        }
    }
    ::LeaveCriticalSection(&this->m_cs);
    return nRead;
}
size_t TcpClient::ShareLink::GetUsed(){
    return totalUsed;
}
void TcpClient::ShareLink::Append(const char* pbuf, size_t size){
    ::EnterCriticalSection(&this->m_cs);
    if (this->IsEmpty())
    {
        pHead = new Node();
        pAppending = pHead;
    }
    int ave = pAppending->Available();
    if (ave > size){
        pAppending->Write(pbuf, size);
    }
    else{
        int rest = size - ave;
        pAppending->Write(pbuf, ave);
        if (rest > 0)
        {
            if (NULL == pAppending->pNext){
                pAppending->pNext = new Node();
            }
            pAppending = pAppending->pNext;
            pAppending->Write(pbuf + ave, rest);
        }
        else
        {// rest == 0
            if (NULL == pAppending->pNext){
                pAppending->pNext = new Node();
            }
            pAppending = pAppending->pNext;
        }
    }
    totalUsed += size;
    ::LeaveCriticalSection(&this->m_cs);
}
void TcpClient::ShareLink::GetAppendingPointer(char** ppBuf, size_t& size){
    ::EnterCriticalSection(&this->m_cs);
    if (this->IsEmpty())
    {
        pHead = new Node();
        pAppending = pHead;
    }
    assert(!pAppending->IsFull());
    *ppBuf = pAppending->cData + pAppending->nStartIndex;
    size = TcpClient::MAX_BUFF_SIZE::MAX_SIZE -(pAppending->nUsed + pAppending->nStartIndex);
    ::LeaveCriticalSection(&this->m_cs);
}
void TcpClient::ShareLink::Read(size_t size){
    ::EnterCriticalSection(&this->m_cs);
    assert(!this->IsEmpty());
    if (pHead->GetUsed() > size){
        pHead->Read(size);
    }
    else{
        Node* t = pHead;
        size_t rest = size - pHead->GetUsed();
        pHead->Read(pHead->GetUsed());
        if (rest > 0){
            pHead = pHead->pNext;
            pHead->Read(rest);
            this->PushFreeNode(t);
        }
        else
        { //rest == 0
            if (pHead->Available() == 0){// just enough
                assert(pHead->pNext);
                pHead = pHead->pNext;
                this->PushFreeNode(t);
            }
        }
    }
    totalUsed -= size;
    ::LeaveCriticalSection(&this->m_cs);
}
void TcpClient::ShareLink::Read(char* pBuf, size_t size){
    ::EnterCriticalSection(&this->m_cs);
    assert(!this->IsEmpty());
    if (pHead->GetUsed() > size){
        pHead->Read(pBuf, size);
    }
    else{
        Node* t = pHead;        
        size_t rest = size - pHead->GetUsed();
        pHead->Read(pBuf, pHead->GetUsed());
        if (rest > 0){
            pHead = pHead->pNext;
            pHead->Read(pBuf + size - rest, rest);
            this->PushFreeNode(t);
        }
        else
        { //rest == 0
            if (pHead->Available() == 0){// just enough
                assert(pHead->pNext);
                pHead = pHead->pNext;
                this->PushFreeNode(t);
            }
        }
    }
    totalUsed -= size;
    ::LeaveCriticalSection(&this->m_cs);
}
void TcpClient::ShareLink::Write(size_t size){
    ::EnterCriticalSection(&this->m_cs);
    assert(!this->IsEmpty());    
    int ave = pAppending->Available();
    if (ave > size){
        pAppending->Write(size);
    }
    else{
        int rest = size - ave;
        pAppending->Write(ave);
        if (rest > 0)
        {
            pAppending = pAppending->pNext;
            assert(NULL != pAppending);
            pAppending->Write(rest);
        }
        else
        {// rest == 0
            if (NULL == pAppending->pNext){
                pAppending->pNext = new Node();
            }
            pAppending = pAppending->pNext;
        }
    }
    totalUsed += size;
    ::LeaveCriticalSection(&this->m_cs);
}
bool TcpClient::ShareLink::IsEmpty(){
    return pHead == NULL;
}
void TcpClient::ShareLink::PushFreeNode(Node* node){
    ::EnterCriticalSection(&this->m_cs);
    if (node){
        node->Reset();
        if (this->IsEmpty())
        {
            pHead = node;
            pAppending = node;
        }
        else
        {
            node->pNext = pAppending->pNext;
            pAppending->pNext = node;
        }        
    }
    ::LeaveCriticalSection(&this->m_cs);
}
TcpClient::Node* TcpClient::ShareLink::PopHeadNode(){
    Node* t = NULL;
    ::EnterCriticalSection(&this->m_cs);
    if (!this->IsEmpty())
    {
        if (totalUsed > 0)
        {
            t = pHead;
            totalUsed -= pHead->nUsed;
            if (pHead == pAppending){
                if (NULL == pAppending->pNext){
                    pAppending = new Node();
                }
                pAppending = pAppending->pNext;
            }
            pHead = pHead->pNext;
        }
    }
    ::LeaveCriticalSection(&this->m_cs);
    return t;
}