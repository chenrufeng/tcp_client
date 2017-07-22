#include "stdafx.h"
#include "SkyParser.h"
#include "windows.h"
static const size_t DECODE_BUFLEN = 0x80000;


SkyParser::SkyParser()
{
    dwInSequence = 0;
    
    DecoderBuffer = new char[DECODE_BUFLEN];
}

SkyParser::~SkyParser()
{
    delete[] DecoderBuffer;
}

void SkyParser::Reset()
{
    ZeroMemory(&Header, sizeof(Header));
}

TcpParser* SkyParser::Clone()
{
    return new SkyParser;
}

void SkyParser::SetHeader(const char* p)
{
    memcpy(&Header, p, sizeof(Header));
}

size_t SkyParser::GetHeaderSize(){
    return sizeof(Header);
}

size_t SkyParser::GetBodySize()
{
    int bodySize = 0;
    if ((Header.paclen == 0) || (Header.paclen >= 0xfff0))
    {
        bodySize = Header.seqnum;
    }
    else
    {
        bodySize = Header.paclen;
    }
    return bodySize;
}

char* SkyParser::GenerateHeaderByBody(const char* pBody, size_t size){
    return 0;
}

char* SkyParser::GetHeader(){
    return (char*)&Header;
}

size_t SkyParser::GetPackSize(){
    return GetHeaderSize() + GetBodySize();
}


bool SkyParser::Encoded()
{
    return ((Header.paclen == 0) || (Header.paclen >= 0xfff0));
}

bool SkyParser::Decode(char* pack, char* &destbuff, size_t &destsize)
{
    char* buffer = pack + GetHeaderSize();
    int totalBytes = GetPackSize() - GetHeaderSize();
    if (Encoded())
    {
        if (Header.paclen == 0xffff)
        {
            decoder.reset();
        }

        // ����汾����Ϊ�˺����з��������м��ݣ�������Ҫ�Խ������ݴ�С�������⴦����
        UINT16 *encoderBeforeSize = (UINT16 *)buffer;
        size_t decoderSize = *encoderBeforeSize;
        decoder.SetEncoderData(buffer + 2, totalBytes - 2);
        size_t size = decoder.lzw_decode(DecoderBuffer, DECODE_BUFLEN);

        if (size != decoderSize)
        {
            // ������Ҫ����Ϊ�������������ѹ�����ݴ�С�� 0x80000�����Ǵ��͵�����ֻ��2���ֽڳ�������ԭʼ���ݴ�С���ضϡ�����
            if ((size & 0xffff) != decoderSize)
            {
                //ErrorHandle( "package decode failure!!!" );
                return false;
            }
        }
        DNPHDR* pDHeader = ((DNPHDR*)(DecoderBuffer + size));
        destbuff = (DecoderBuffer + size + sizeof(DNPHDR));
        destsize = pDHeader->paclen;
    }

    return true;
}

bool SkyParser::Encode(char* body, char* &destbuff, size_t &destsize)
{
    destbuff = body;
    destsize = GetPackSize();
    return TRUE;
}