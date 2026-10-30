#pragma once
#ifdef ENABLE_NETWORK
#include "IFile.h"

#pragma pack(push, 1)
struct OLChapterInfo
{
    uint32_t index;
    uint32_t titleOffset;
    uint32_t urlOffset;
    uint32_t textLen;
};

struct OLHeader : public HeaderBase
{
    uint32_t bookNameOffset;
    uint32_t mainPageOffset;
    uint32_t hostOffset;
    uint64_t updateTime;
    uint32_t isFinished; // for bookstatus, deprecated
    uint32_t notBooksrc : 1;
    uint32_t reserve1   : 31;
    uint32_t reserve2[3]; // reserve
    uint32_t chapterSize;
    OLChapterInfo chapters[1];
};
#pragma pack()

class OLFile : public IFile
{
public:
    // for generate OLHeader
    OLFile(const std::wstring& bookName, const std::string& mainPage, const std::string& host, const Chapters& chapters = {}, bool notBooksrc = false, bool isFinished = false);

    // for parse OLHeader
    OLFile(const char* buffer, size_t size, bool ownBuffer = false);
    virtual const wchar_t* bookName() const;
    virtual Chapters chapters() const;
    const char* mainPage() const;
    const char* host() const;

    // common
    const OLHeader* header() const;

    // for free
    virtual ~OLFile();
};
#endif