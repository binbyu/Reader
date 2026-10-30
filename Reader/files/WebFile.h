#pragma once
#ifdef ENABLE_NETWORK
#include "IFile.h"

#pragma pack(push, 1)
struct WebChapterInfo
{
    uint32_t index;
    uint32_t titleOffset;
    uint32_t textLen;
};

struct WebHeader : public HeaderBase
{
    uint32_t bookNameOffset;
    uint32_t bookMetaOffset;
    uint32_t reserve[4];
    uint32_t chapterSize;
    WebChapterInfo chapters[1];
};
#pragma pack()

class WebFile : public IFile
{
public:
    // for generate WebHeader
    WebFile(const std::wstring& bookName, const WebShelfItem& bookMeta, const Chapters& chapters = {});

    // for parse WebHeader
    WebFile(const char* buffer, size_t size, bool ownBuffer = false);
    virtual const wchar_t* bookName() const;
    virtual Chapters chapters() const;
    const char* bookMetaJson() const;
    WebShelfItem* bookMeta();

    // common
    const WebHeader* header() const;

    // for free
    virtual ~WebFile();

protected:
    WebShelfItem* m_item;
};
#endif