#pragma once
class TcpParser
{
public:
    TcpParser();
    ~TcpParser();
    virtual size_t GetHeaderSize();
    // return header pointer
    virtual char* GenerateHeaderByBody(const char* pBody, size_t size);
    virtual char* GetHeader();
    //if the stream has not headers or separators. it return 0.
    virtual size_t GetPackSize();
};

