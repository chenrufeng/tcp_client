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
    m_sendbufHead.CleanUp();
    m_recvbufHead.CleanUp();
    m_sendbufHead.SetParser(&m_parser);
    m_recvbufHead.SetParser(&m_parser);
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
    return m_recvbufHead.ReadMsg(pBuf, buff_size);
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
