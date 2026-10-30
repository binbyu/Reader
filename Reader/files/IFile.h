#pragma once
#ifdef ENABLE_NETWORK
#include <string>
#include <vector>
#include "types.h"

#pragma pack(push, 1)
struct HeaderBase
{
#if 0 // OLFile header is relesed, but not used now
    uint32_t magic;  // ".WEB" , ".OL"
    uint32_t version;
    uint32_t crc;
#endif
    uint32_t headerSize;
};
#pragma pack()

class IFile
{
public:
    IFile();
    IFile(const char* buffer, size_t size, bool ownBuffer = false);
    virtual ~IFile();
    virtual const bool isValid() const;
    virtual const wchar_t* bookName() const = 0;
    virtual Chapters chapters() const = 0;

    const char* buffer() const;
    size_t size() const;

    const HeaderBase* header() const;

protected:
    uint32_t crc32() const;

protected:
    char*  m_buffer;
    size_t m_size;
    bool   m_ownBuffer;
};
#endif
