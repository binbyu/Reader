#ifdef ENABLE_NETWORK
#include "IFile.h"

IFile::IFile()
    : m_buffer(nullptr)
    , m_size(0)
    , m_ownBuffer(false)
{
}

IFile::IFile(const char* buffer, size_t size, bool ownBuffer)
    : m_buffer((char*)buffer)
    , m_size(size)
    , m_ownBuffer(ownBuffer)
{
}

IFile::~IFile()
{
    if (m_ownBuffer && m_buffer)
    {
        free(m_buffer);
        m_buffer = nullptr;
    }
}

const bool IFile::isValid() const
{
    HeaderBase* hdr = (HeaderBase*)m_buffer;
    return m_buffer && m_size >= sizeof(HeaderBase) && hdr->headerSize <= m_size;
}

const char* IFile::buffer() const
{
    return m_buffer;
}

size_t IFile::size() const
{
    return m_size;
}

const HeaderBase* IFile::header() const
{
    return (HeaderBase*)m_buffer;
}

uint32_t IFile::crc32() const
{
    const uint32_t CRC32_POLY = 0xEDB88320U;

    if (!m_buffer || m_size < sizeof(uint32_t)) return 0U;

    uint32_t crc = 0xFFFFFFFFU;
    size_t i;
    int j;
    size_t offset = sizeof(HeaderBase) - sizeof(uint32_t);
    const unsigned char* data = (unsigned char*)m_buffer + offset;
    size_t size = m_size - offset;

    for (i = 0; i < size; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ CRC32_POLY;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFFU;
}
#endif