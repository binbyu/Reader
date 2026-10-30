#ifdef ENABLE_NETWORK
#include "WebFile.h"
#ifndef NO_JSON // use for olweb2txt tool
#include "JsonSerialize.h"
#endif

WebFile::WebFile(const std::wstring& bookName, const WebShelfItem& bookMeta, const Chapters& chapters)
    : IFile(nullptr, 0, true)
    , m_item(nullptr)
{
    std::vector<uint8_t> meta;
#ifndef NO_JSON
    JsonSerialize::enserialize(bookMeta, meta);
#endif

    size_t baseSize = sizeof(WebHeader) - sizeof(WebChapterInfo) + sizeof(WebChapterInfo) * chapters.size();
    size_t bookNameSize = (bookName.size() + 1) * sizeof(wchar_t);
    size_t bookMetaSize = meta.size() + 1;

    m_size = baseSize + bookNameSize + bookMetaSize;
    for (auto& chapter : chapters)
    {
        m_size += (chapter.title.size() + 1) * sizeof(wchar_t);
    }

    m_buffer = (char*)malloc(m_size);
    memset(m_buffer, 0, m_size);
    WebHeader* header = (WebHeader*)m_buffer;
    header->headerSize = (uint32_t)m_size;
    header->bookNameOffset = (uint32_t)baseSize;
    baseSize += bookNameSize;
    header->bookMetaOffset = (uint32_t)baseSize;
    baseSize += bookMetaSize;
    header->chapterSize = (uint32_t)chapters.size();
    for (size_t i = 0; i < chapters.size(); i++)
    {
        WebChapterInfo* chapter = &header->chapters[i];
        chapter->index = chapters[i].index;
        chapter->textLen = chapters[i].textLen;
        chapter->titleOffset = (uint32_t)baseSize;
        baseSize += (chapters[i].title.size() + 1) * sizeof(wchar_t);
    }

    memcpy(m_buffer + header->bookNameOffset, bookName.c_str(), bookNameSize);
    memcpy(m_buffer + header->bookMetaOffset, meta.data(), meta.size());
    for (size_t i = 0; i < chapters.size(); i++)
    {
        WebChapterInfo* chapter = &header->chapters[i];
        memcpy(m_buffer + chapter->titleOffset, chapters[i].title.c_str(), (chapters[i].title.size() + 1) * sizeof(wchar_t));
    }
}

WebFile::WebFile(const char* buffer, size_t size, bool ownBuffer)
    : IFile(buffer, size, ownBuffer)
    , m_item(nullptr)
{
}

const wchar_t* WebFile::bookName() const
{
    return (wchar_t*)(m_buffer + header()->bookNameOffset);
}

const char* WebFile::bookMetaJson() const
{
    return (char*)(m_buffer + header()->bookMetaOffset);
}

WebShelfItem* WebFile::bookMeta()
{
    if (!m_item)
    {
        const char* json = bookMetaJson();
        size_t jsonLen = strlen(json);
        std::vector<uint8_t> bytes;
        bytes.assign(json, json + jsonLen);

        m_item = new WebShelfItem;
#ifndef NO_JSON
        if (!JsonSerialize::deserialize(bytes, *m_item))
#endif
        {
            delete m_item;
            m_item = nullptr;
        }
    }
    return m_item;
}

Chapters WebFile::chapters() const
{
    Chapters chapters;
    const WebHeader* hdr = header();
    for (size_t i = 0; i < hdr->chapterSize; i++)
    {
        const WebChapterInfo* chapter = &hdr->chapters[i];
        Chapter c;
        c.index = chapter->index;
        c.textLen = chapter->textLen;
        c.title = (wchar_t*)(m_buffer + chapter->titleOffset);
        c.titleLineLen = (int)c.title.size();
        chapters.push_back(c);
    }
    return chapters;
}

const WebHeader* WebFile::header() const
{
    return (WebHeader*)m_buffer;
}

WebFile::~WebFile()
{
    if (m_item)
    {
        delete m_item;
        m_item = nullptr;
    }
}
#endif