#include "stdafx.h"
#include "TcpParser.h"
#include "windows.h"
TcpParser::TcpParser()
{
}


TcpParser::~TcpParser()
{
}

void TcpParser::Reset()
{
    //RESET HEADER DATA
}

TcpParser* TcpParser::Clone()
{
    return new TcpParser;
}

size_t TcpParser::GetHeaderSize(){
    return 0;
}

char* TcpParser::GenerateHeaderByBody(const char* pBody, size_t size){
    return 0;
}

void TcpParser::SetHeader(const char* p)
{
    // COPY p to header data structure.
}

char* TcpParser::GetHeader(){
    return 0;
}

size_t TcpParser::GetBodySize(){
    return  0; // 0 represent body size.
}

size_t TcpParser::GetPackSize(){
    return GetHeaderSize() + GetBodySize(); // 0 represent body size.
}

bool TcpParser::Encoded()
{
    return false;
}

bool TcpParser::Decode(char* pack, char* &destbuff, size_t &destsize)
{
    return true;
}

bool TcpParser::Encode(char* body, char* &destbuff, size_t &destsize)
{
    return true;
}

