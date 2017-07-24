#pragma once
class TcpParser
{
public:
    TcpParser();
    virtual ~TcpParser();
    void Reset();
    virtual TcpParser* Clone();
    virtual void SetHeader(const char* p);
    virtual size_t GetHeaderSize();
    virtual size_t GetBodySize();
    virtual void SetBodySize(int n);
    //if the stream has not headers or separators. it return 0.
    virtual size_t GetPackSize();

    // return header pointer
    virtual char* GenerateHeaderByBody(const char* pBody, size_t size);
    virtual char* GetHeader();
    
    virtual bool Encoded();
    virtual bool Decode(char* pack, char* &destbuff, size_t &destsize);
    virtual bool Encode(char* body, char* &destbuff, size_t &destsize);
protected:
    // HEADER STRUCT    
  
};
