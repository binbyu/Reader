#ifdef ENABLE_NETWORK
#include "OLFile.h"
#include <time.h>

OLFile::OLFile(const std::wstring& bookName, const std::string& mainPage, const std::string& host, const Chapters& chapters, bool notBooksrc, bool isFinished)
    : IFile(nullptr, 0, true)
{
    size_t baseSize = sizeof(OLHeader) - sizeof(OLChapterInfo) + sizeof(OLChapterInfo) * chapters.size();
    size_t bookNameSize = (bookName.size() + 1) * sizeof(wchar_t);
    size_t mainPageSize = mainPage.size() + 1;
    size_t hostSize = host.size() + 1;

    m_size = baseSize + bookNameSize + mainPageSize + hostSize;
    for (auto& chapter : chapters)
    {
        m_size += (chapter.title.size() + 1) * sizeof(wchar_t);
        m_size += chapter.url.size() + 1;
    }

    m_buffer = (char*)malloc(m_size);
    memset(m_buffer, 0, m_size);
    OLHeader* header = (OLHeader*)m_buffer;
    header->headerSize = (uint32_t)m_size;
    header->bookNameOffset = (uint32_t)baseSize;
    baseSize += bookNameSize;
    header->mainPageOffset = (uint32_t)baseSize;
    baseSize += mainPageSize;
    header->hostOffset = (uint32_t)baseSize;
    baseSize += hostSize;
    header->updateTime = time(NULL);
    header->isFinished = isFinished ? 1 : 0;
    header->notBooksrc = notBooksrc ? 1 : 0;
    header->chapterSize = (uint32_t)chapters.size();
    for (size_t i = 0; i < chapters.size(); i++)
    {
        OLChapterInfo* chapter = &header->chapters[i];
        chapter->index = chapters[i].index;
        chapter->textLen = chapters[i].textLen;
        chapter->titleOffset = (uint32_t)baseSize;
        baseSize += (chapters[i].title.size() + 1) * sizeof(wchar_t);
        chapter->urlOffset = (uint32_t)baseSize;
        baseSize += chapters[i].url.size() + 1;
    }

    memcpy(m_buffer + header->bookNameOffset, bookName.c_str(), bookNameSize);
    memcpy(m_buffer + header->mainPageOffset, mainPage.c_str(), mainPageSize);
    memcpy(m_buffer + header->hostOffset, host.c_str(), hostSize);
    for (size_t i = 0; i < chapters.size(); i++)
    {
        OLChapterInfo* chapter = &header->chapters[i];
        memcpy(m_buffer + chapter->titleOffset, chapters[i].title.c_str(), (chapters[i].title.size() + 1) * sizeof(wchar_t));
        memcpy(m_buffer + chapter->urlOffset, chapters[i].url.c_str(), chapters[i].url.size() + 1);
    }
}

OLFile::OLFile(const char* buffer, size_t size, bool ownBuffer)
    : IFile(buffer, size, ownBuffer)
{
}

const wchar_t* OLFile::bookName() const
{
    return (wchar_t*)(m_buffer + header()->bookNameOffset);
}

const char* OLFile::mainPage() const
{
    return (char*)(m_buffer + header()->mainPageOffset);
}

const char* OLFile::host() const
{
    return (char*)(m_buffer + header()->hostOffset);
}

Chapters OLFile::chapters() const
{
    Chapters chapters;
    const OLHeader* hdr = header();
    for (size_t i = 0; i < hdr->chapterSize; i++)
    {
        const OLChapterInfo* chapter = &hdr->chapters[i];
        Chapter c;
        c.index = chapter->index;
        c.textLen = chapter->textLen;
        c.title = (wchar_t*)(m_buffer + chapter->titleOffset);
        c.url = (char*)(m_buffer + chapter->urlOffset);
        c.titleLineLen = (int)c.title.size();
        chapters.push_back(c);
    }
    return chapters;
}

const OLHeader* OLFile::header() const
{
    return (OLHeader*)m_buffer;
}

OLFile::~OLFile()
{
}
#endif