#include "stdafx.h"
#include "ShareLink.h"
#include "WinBase.h"
#include "assert.h"
Node::Node(){
    Reset();
}
Node::~Node(){
}
void Node::Write(size_t size){
    nUsed += size;
}
void Node::Write(const char* buf, size_t size){
    memcpy(cData + nStartIndex + nUsed, buf, size);
    nUsed += size;
}
void Node::Read(size_t size){
    nUsed -= size;
    nStartIndex += size;
}
void Node::Read(char* pBuf, size_t size){
    memcpy(pBuf, cData + nStartIndex, size);
    nUsed -= size;
    nStartIndex += size;
}
void Node::Peek(char* pBuf, size_t size){
    memcpy(pBuf, cData + nStartIndex, size);
}
bool Node::IsEmpty(){
    return (this->nUsed <= 0);
}
bool Node::IsFull(){
    return Available() == 0;
}
char* Node::GetData(){
    return cData + nStartIndex;
}
size_t Node::Available(){
    return MAX_SIZE - nStartIndex - nUsed;
}
size_t Node::GetUsed(){
    return this->nUsed;
}
ShareLink::ShareLink(){
    pHead = NULL;
    pAppending = NULL;
    totalUsed = 0;
    m_parser = NULL;
    ::InitializeCriticalSection(&this->m_cs);
}
ShareLink::~ShareLink(){

    ::DeleteCriticalSection(&this->m_cs);
}

void ShareLink::CleanUp(){
    while (pHead)
    {
        Node* t = pHead->pNext;
        delete pHead;
        pHead = t;
    }
    pHead = NULL;
    pAppending = NULL;
}
void ShareLink::SetParser(TcpParser* parser){
    m_parser = parser;
}
bool ShareLink::Peek(char* pbuf, size_t size){
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
size_t ShareLink::ReadMsg(char* pBuf, size_t& size){
    size_t nRead = 0;
    ::EnterCriticalSection(&this->m_cs);
    if (this->Peek(m_parser->GetHeader(), m_parser->GetHeaderSize()))
    {
        if (this->GetUsed() >= m_parser->GetPackSize()){
            if (0 == m_parser->GetPackSize())
            {// no separator
                nRead = min(size, this->GetUsed());
                if (nRead > 0)
                {
                    this->Read(pBuf, nRead);
                }
            }
            else
            {
                nRead = m_parser->GetPackSize() - m_parser->GetHeaderSize();
                this->Read(m_parser->GetHeader(), m_parser->GetHeaderSize());
                this->Read(pBuf, nRead);
            }
        }
    }
    ::LeaveCriticalSection(&this->m_cs);
    return nRead;
}
size_t ShareLink::GetUsed(){
    return totalUsed;
}
void ShareLink::Append(const char* pbuf, size_t size){
    ::EnterCriticalSection(&this->m_cs);
    // adding header.
    m_parser->GenerateHeaderByBody(pbuf, size);
    size_t nHeader = m_parser->GetHeaderSize();
    if (nHeader > 0){
        this->Append_Imp(m_parser->GetHeader(), nHeader);
    }
    this->Append_Imp(pbuf, size);
    ::LeaveCriticalSection(&this->m_cs);
}
void ShareLink::Append_Imp(const char* pbuf, size_t size){
    ::EnterCriticalSection(&this->m_cs);
    if (this->IsEmpty())
    {
        pHead = new Node();
        pAppending = pHead;
    }
    size_t ave = pAppending->Available();
    if (ave > size ){
        pAppending->Write(pbuf, size);
    }
    else{
        size_t rest = size - ave;
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
void ShareLink::GetAppendingPointer(char** ppBuf, size_t& size){
    ::EnterCriticalSection(&this->m_cs);
    if (this->IsEmpty())
    {
        pHead = new Node();
        pAppending = pHead;
    }
    assert(!pAppending->IsFull());
    *ppBuf = pAppending->cData + pAppending->nStartIndex;
    size = MAX_BUFF_SIZE::MAX_SIZE - (pAppending->nUsed + pAppending->nStartIndex);
    ::LeaveCriticalSection(&this->m_cs);
}
void ShareLink::Read(size_t size){
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
void ShareLink::Read(char* pBuf, size_t size){
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
void ShareLink::Write(size_t size){
    ::EnterCriticalSection(&this->m_cs);
    assert(!this->IsEmpty());
    size_t ave = pAppending->Available();
    if (ave > size){
        pAppending->Write(size);
    }
    else{
        size_t rest = size - ave;
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
bool ShareLink::IsEmpty(){
    return pHead == NULL;
}
void ShareLink::PushFreeNode(Node* node){
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
Node* ShareLink::PopHeadNode(){
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