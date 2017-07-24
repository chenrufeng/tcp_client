#include "stdafx.h"
#include "SkyParser.h"



static const size_t DECODE_BUFLEN = 0x80000;


SkyParser::SkyParser()
{
    dwInSequence = 0;
    m_dwKey = 0;
    DecoderBuffer = new char[DECODE_BUFLEN];
    m_dwOutSequence = 0;
    m_bUseVerify = TRUE;
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

void  SkyParser::SetBodySize(int n)
{
    Header.paclen = n;    
}

char* SkyParser::GenerateHeaderByBody(const char* pBody, size_t size){
    Header.paclen = size;
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

        // 这个版本就是为了和现有服务器进行兼容，但是需要对解码数据大小进行特殊处理。。
        UINT16 *encoderBeforeSize = (UINT16 *)buffer;
        size_t decoderSize = *encoderBeforeSize;
        decoder.SetEncoderData(buffer + 2, totalBytes - 2);
        size_t size = decoder.lzw_decode(DecoderBuffer, DECODE_BUFLEN);

        if (size != decoderSize)
        {
            // 这里主要是因为服务器上允许的压缩数据大小是 0x80000，但是传送的数据只有2个字节长，导致原始数据大小被截断。。。
            if ((size & 0xffff) != decoderSize)
            {
                //ErrorHandle( "package decode failure!!!" );
                return false;
            }
        }
        DNPHDR* pDHeader = ((DNPHDR*)(DecoderBuffer));
        destbuff = (DecoderBuffer + sizeof(DNPHDR));
        destsize = pDHeader->paclen;
    }

    return true;
}

struct Encryptor
{
    Encryptor(iDirectNetCryption *encryptor) : encryptor(encryptor) {}
    operator bool() { return encryptor != NULL; }
    UINT32 crc32(void *data, DWORD size) { return encryptor->CRC32_compute(data, size); }
    void encrypt(void *data, DWORD size) { encryptor->DES_encrypt(data, size); }
    iDirectNetCryption *encryptor;
};


bool SkyParser::Encode(char* pvBuf, char* &destbuff, size_t &destsize)
{
    int data_size = Header.paclen;
    char* raw_buffer = m_SendBuffer;
    char* data = NULL; // 数据
    if (m_bUseVerify)
    {
        size_t wSize = Header.paclen;
        data = m_SendBuffer + 16;
        LPDWORD pDword = (LPDWORD)data;

        memcpy(data + 8, pvBuf, wSize);

        // 4字节上对齐
        if (wSize & 0x3)
            wSize = (wSize & 0xfffc) + 4;

        pDword[1] = m_dwKey = (m_dwKey == 0)
            ? 897433309//(timeGetTime() ^ ((rand() << 20) | (rand() << 10) | rand()))
            : (_get_dval(m_dwKey));

        pDword[0] = cryption.CRC32_compute(data + 8, wSize) ^ m_dwKey;

        LPDWORD pXorPtr = (LPDWORD)data + 2;
        WORD count = (wSize >> 2);

        DWORD dwAccKey = m_dwKey;

        while (count > 0)
        {
            *pXorPtr ^= dwAccKey;
            pXorPtr++;
            count--;
            dwAccKey += 0xcdcd;
        }

        cryption.DES_encrypt(data, 8);
        data_size = Header.paclen + 8;
        
    }
    else{
        data = pvBuf;
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////


   
    {
        // 设定，一个打包数据的最大长度为 MAX_BUFFER_SIZE * 4
        size_t new_size = (0 + data_size + sizeof(CRYPTION));
        size_t size = 0;
        

        reinterpret_cast<CRYPTION*>(&raw_buffer[size])->dnphdr.seqnum = (WORD)m_dwOutSequence;
        reinterpret_cast<CRYPTION*>(&raw_buffer[size])->dnphdr.paclen = (UINT16)data_size;
        m_dwOutSequence++;

        UINT32 u32 = 111;// = GetTickCount();
        UINT64 u64 = 333; //; QueryPerformanceCounter((LARGE_INTEGER*)&u64);
        reinterpret_cast<CRYPTION*>(&raw_buffer[size])->signature = ((u64 << 32) | u32);
        Encryptor encryptor = Encryptor(&cryption);
        if (encryptor)
        {
            reinterpret_cast<CRYPTION*>(&raw_buffer[size])->crc32 =
                encryptor.crc32((char*)&raw_buffer[size] + sizeof(((CRYPTION*)0)->crc32),
                sizeof(CRYPTION) - sizeof(((CRYPTION*)0)->crc32));

            encryptor.encrypt(&raw_buffer[size], sizeof(CRYPTION));
        }

        size += sizeof(CRYPTION);

        memcpy(&raw_buffer[size], data, data_size);

        destbuff = (char*)raw_buffer;
        destsize = data_size + size;
    }
    return TRUE;
}