#pragma once
class TcpParser
{
public:
    TcpParser();
    ~TcpParser();
    virtual size_t GetHeaderSize();
    virtual char* GetHeader();
    virtual int GetBodySize();    
};

