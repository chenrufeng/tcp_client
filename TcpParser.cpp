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
char* TcpParser::GetHeader(){
    return NULL;
}
int TcpParser::GetBodySize(){
    return 0;
}