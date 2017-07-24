#pragma once
#include "DirectNet.h"
#include "windows.h"
#include "lzw.hpp"
#include "TcpParser.h"
#define USE_CRC
#include "desenc.h"
// the DirectNetProtocol header
struct DNPHDR {
    UINT16 seqnum;
    UINT16 paclen;
};

// the header that is passed one-way from client to server
struct CRYPTION {
    UINT32 crc32;
    DNPHDR dnphdr;
    UINT64 signature;
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
    virtual void   SetBodySize(int n);
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
    DWORD m_dwKey;
    int dwInSequence;
    LPSTR DecoderBuffer;
    DWORD m_dwOutSequence;
    char m_SendBuffer[0xffff];
    cDirectNetEncryption cryption;
    BOOL m_bUseVerify;
};


