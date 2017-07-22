#pragma once
#include "lzw.hpp"
#include "TcpParser.h"

// the DirectNetProtocol header
struct DNPHDR {
    UINT16 seqnum;
    UINT16 paclen;
};


class SkyParser : public TcpParser
{
public:
    SkyParser();
    virtual ~SkyParser();
    void Reset();
    virtual TcpParser* Clone();
    virtual void SetHeader(const char* p);
    virtual size_t GetHeaderSize();
    virtual size_t GetBodySize();
    //if the stream has not headers or separators. it return 0.
    virtual size_t GetPackSize();

    virtual char* GetHeader();
    // return header pointer
    virtual char* GenerateHeaderByBody(const char* pBody, size_t size);
    

    virtual bool Encoded();
    virtual bool Decode(char* pack, char* &destbuff, size_t &destsize);
    virtual bool Encode(char* body, char* &destbuff, size_t &destsize);

    
private:
    DNPHDR Header;
    lzw::lzwDecoder decoder;
    int dwInSequence;
    LPSTR DecoderBuffer;
};


