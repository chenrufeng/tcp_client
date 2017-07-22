#include "stdafx.h"
#include "TcpClient.h"
#include "WinBase.h"
#include "assert.h"
#pragma comment(lib,"ws2_32.lib")  

TcpClient::TcpClient()
{
    ::InitializeCriticalSection(&SendCS);
}
TcpClient::~TcpClient()
{
    ::DeleteCriticalSection(&SendCS);
}
void TcpClient::Init(const char* ip, int port, size_t max_pack_size, TcpParser* parser)
{
    //网络初始化  
    WSADATA wsaData;
    WSAStartup(MAKEWORD(1, 1), &wsaData);
    memcpy(this->m_ip, ip, strlen(ip) + 1);
    Node::MaxSize = max_pack_size;
    this->m_port = port;
    this->m_connected = false;
    this->m_ThreadHandle = NULL;
    this->m_sock = INVALID_SOCKET;
    this->m_stop = 0;
    m_max_pack_size = max_pack_size;

    if (parser == NULL)
    {
        this->GetmsgParser = new TcpParser;
        this->SendmsgParser = new TcpParser;
    }
    else{
        this->GetmsgParser = parser->Clone();
        this->SendmsgParser = parser->Clone();
    }
}
bool TcpClient::Connect(const char* ip, int port)
{
    memcpy(this->m_ip, ip, strlen(ip) + 1);
    this->m_port = port;
    return this->Connect();
}

bool TcpClient::Connect()
{
    m_sendbufHead.CleanUp();
    m_recvbufHead.CleanUp();
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
    if (m_recvbufHead.Peek(GetmsgParser->GetHeader(), GetmsgParser->GetHeaderSize()))
    {
        if (m_recvbufHead.Peek(pBuf, GetmsgParser->GetPackSize()))
        {
            m_recvbufHead.Read(GetmsgParser->GetPackSize());
            return GetmsgParser->GetPackSize();
        }
    }
    return 0;
}
void TcpClient::Send(void* pbuff, size_t size)
{
    if (size < 1) return;
    assert(size <= m_max_pack_size);
    ::EnterCriticalSection(&SendCS);
    SendmsgParser->GenerateHeaderByBody((char*)pbuff, size);    
    this->m_sendbufHead.Append_Imp(SendmsgParser->GetHeader(), SendmsgParser->GetHeaderSize());    
    this->m_sendbufHead.Append_Imp((char*)pbuff, size);    
    ::LeaveCriticalSection(&SendCS);
}
void TcpClient::Stop()
{
    InterlockedIncrement(&this->m_stop);

}
void TcpClient::CleanUp()
{
    if (this->GetmsgParser){
        delete this->GetmsgParser;
        this->GetmsgParser = NULL;
    }
    if (this->SendmsgParser){
        delete this->SendmsgParser;
        this->SendmsgParser = NULL;
    }
    m_sendbufHead.CleanUp();
    m_recvbufHead.CleanUp();
    WSACleanup();
}
DWORD WINAPI TcpClient::ThreadFunc(LPVOID lpParameter){
    TcpClient* mine = (TcpClient*)lpParameter;
    FD_SET writeSet;
    FD_SET readSet;
    timeval timeout = { 1, 0 };
    int total;
    TcpParser* writeParser = mine->SendmsgParser->Clone();
    TcpParser* readParser = mine->GetmsgParser->Clone();
    Node* sending_buf = new Node;
    Node* recving_buf = new Node;
    char* pSendingPointer = NULL;
    size_t  nSendingRemain = 0;
    while (mine->m_stop == 0)
    {
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        FD_SET(mine->m_sock, &readSet);
        FD_SET(mine->m_sock, &writeSet);

        if (nSendingRemain <= 0 && mine->m_sendbufHead.Peek(writeParser->GetHeader(), writeParser->GetHeaderSize()))
        {
            // should be ok
            if (mine->m_sendbufHead.Peek(sending_buf->GetData(), writeParser->GetPackSize()))
            {
                writeParser->Encode(sending_buf->GetData() + writeParser->GetHeaderSize(), pSendingPointer, nSendingRemain);
            }
            else
            {
                break;
            }
        }
        
        if (nSendingRemain > 0)
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
                size_t req_recv_size = readParser->GetHeaderSize();
                if (recving_buf->GetUsed() == req_recv_size)
                {
                    req_recv_size = readParser->GetBodySize();
                }
                else if (recving_buf->GetUsed() > req_recv_size)
                {                    
                    req_recv_size = readParser->GetPackSize() - recving_buf->GetUsed();
                }
                
                if (req_recv_size > Node::MaxSize - recving_buf->GetUsed())
                {
                    //printf("recieved a bad header: %d\n", req_recv_size);
                    break;
                }
                size_t recvn = 0;
                recvn = recv(mine->m_sock, recving_buf->GetData(), req_recv_size, 0);
                //if the connection has been gracefully closed, the return value is zero.
                if (recvn == 0) {
                    //printf("recv failed: %d\n", WSAGetLastError());
                    break;
                }
                if (recvn == SOCKET_ERROR) {
                    //printf("recv failed: %d\n", WSAGetLastError());
                    break;
                }
                // 1.recv head and body of data
                recving_buf->Write(recvn);             
                if (recving_buf->GetUsed() > readParser->GetHeaderSize())
                {
                    readParser->SetHeader(recving_buf->GetData());
                    // 2.check whether have a full pack.
                    if (readParser->GetPackSize() == recving_buf->GetUsed())
                    {
                        // 3.decode pack.
                        char* pDecodedBuf = NULL;
                        size_t decodeSize = 0;
                        readParser->Decode(recving_buf->GetData(), pDecodedBuf, decodeSize);
                        // 4.put pack into recieving queue.
                        mine->m_recvbufHead.Append_Imp(pDecodedBuf, decodeSize);
                        mine->DataArrial(decodeSize);
                        // 5.reset
                        recving_buf->Reset();
                        readParser->Reset();
                    }
                }

            }
#pragma endregion
#pragma region socket send
            if (FD_ISSET(mine->m_sock, &writeSet))
            {
                //1.If processing a connect call(nonblocking), connection has succeeded.
                //2.Data can be sent.
                if (nSendingRemain > 0)
                {
                    size_t bytes_sent = send(mine->m_sock, 
                                            (const char*)pSendingPointer + (writeParser->GetPackSize() - nSendingRemain), 
                                            nSendingRemain, 0);
                    if (bytes_sent == SOCKET_ERROR) {
                        size_t ret_err = WSAGetLastError();
                        if (WSAEWOULDBLOCK != ret_err)
                        {
                            //printf("send failed: %d\n", ret_err);
                            break;
                        }
                    }
                    nSendingRemain -= bytes_sent;
                    if (nSendingRemain <= 0)
                    {
                        writeParser->Reset();
                    }
                }
            }
#pragma endregion
        }
    }    
    if (recving_buf)
    {
        delete recving_buf;
        recving_buf = NULL;
    }
    if (writeParser){
        delete writeParser;
        writeParser = NULL;
    }
    if (readParser){
        delete readParser;
        readParser = NULL;
    }
    mine->m_connected = false;
    return 0;
}


void TcpClient::DataArrial(int size)
{

}