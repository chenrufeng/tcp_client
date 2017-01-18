#include "stdafx.h"
#include "TcpParser.h"


TcpParser::TcpParser()
{
}


TcpParser::~TcpParser()
{
}

size_t TcpParser::GetHeaderSize(){
    return 0;
}

char* TcpParser::GenerateHeaderByBody(const char* pBody, size_t size){
    return NULL;
}

char* TcpParser::GetHeader(){
    return NULL;
}

size_t TcpParser::GetPackSize(){
    return GetHeaderSize() + 0; // 0 represent body size.
}