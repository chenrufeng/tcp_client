#include "stdafx.h"
#include "ShareLink.h"
#include "WinBase.h"
#include "assert.h"

size_t  Node::MaxSize = 0;
Node::Node(){
    cData = new char[MaxSize];
    Reset();
}
Node::~Node(){
    if (cData){
        delete[] cData;
        cData = NULL;
    }
}
void Node::Write(size_t size){
    nUsed += size;
}
void Node::Write(const char* buf, size_t size){
    if (buf)
    {
        memcpy(cData + nStartIndex + nUsed, buf, size);
    }
    nUsed += size;
}
void Node::Read(size_t size){
    nUsed -= size;
    nStartIndex += size;
}
void Node::Read(char* pBuf, size_t size){
    if (pBuf)
    {
        memcpy(pBuf, cData + nStartIndex, size);
    }
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
    return MaxSize - nStartIndex - nUsed;
}
size_t Node::GetUsed(){
    return this->nUsed;
}
ShareLink::ShareLink(){
    pHead = NULL;
    pAppending = NULL;
    totalUsed = 0;
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
                if (rest > 0)
                {
                    pHead->pNext->Peek(pbuf + pHead->GetUsed(), rest);
                }
            }
        }
        ok = true;
    }
    ::LeaveCriticalSection(&this->m_cs);
    return ok;
}

size_t ShareLink::GetUsed(){
    return totalUsed;
}
void ShareLink::AppendPack(const char* pbuf, size_t size, const char* pbuf2, size_t size2){
    if (size <= 0 && size2 <= 0) return;
    ::EnterCriticalSection(&this->m_cs);
    Append_Imp(pbuf, size);  // header
    Append_Imp(pbuf2, size2);// body
    ::LeaveCriticalSection(&this->m_cs);
}

void ShareLink::Append_Imp(const char* pbuf, size_t size){
    if (size <= 0) return;
    //::EnterCriticalSection(&this->m_cs);
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
    //::LeaveCriticalSection(&this->m_cs);
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