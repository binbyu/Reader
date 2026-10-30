#pragma once
#include "BookBase.h"
#include "BookText.h"
#include "BookEpub.h"
#include "BookMobi.h"
#include "BookOnline.h"
#include "conv.h"
#include "Utils.h"
#include "Logger.h"
#include "framework.h"

BookBase::BookBase(const std::wstring& fileName)
    : m_text(nullptr)
    , m_length(0)
    , m_bookInfo(nullptr)
    , m_fileName(fileName)
    , m_chapterRule(nullptr)
    , m_cover(nullptr)
    , m_abort(false)
{
    m_bookName = Utils::fileName(m_fileName);
}

BookBase::~BookBase()
{
    if (m_text)
    {
        free(m_text);
        m_text = nullptr;
    }
    m_length = 0;
    freeCover();
    m_chapters.clear();
}

int BookBase::length() const
{
    return m_length;
}

const wchar_t* BookBase::text() const
{
    return m_text;
}

bool BookBase::isTitleLine(int index) const
{
    for (const auto& i : m_chapters)
    {
        if (index >= i.index)
        {
            if (index < i.index + i.titleLineLen)
            {
                return true;
            }
        }
        else
        {
            break;
        }
    }
    return false;
}

bool BookBase::isChapterStart(int index) const
{
    if (index < 0 || index >= m_length) return false;
    for (const auto& i : m_chapters)
    {
        if (i.index == -1) continue;
        if (index == i.index)
            return true;
        if (index < i.index)
            break;
    }
    return false;
}

bool BookBase::isChapterContinuous(bool isNext, int index) const
{
    if (!isFetchSupport()) return true;
    if (index < 0 || index >= m_length) return true;
    for (size_t i = 0; i < m_chapters.size(); i++)
    {
        if (m_chapters[i].index == -1) continue;
        if (m_chapters[i].index == index) // found
        {
            // isNext = true  is pagedown/linedown
            // isNext = false is pageup/lineup
            size_t target = isNext ? i - 1 : i + 1;
            if (target >= 0 && target < m_chapters.size())
            {
                return m_chapters[target].index != -1;
            }
            else
            {
                return true;
            }
        }
        if (m_chapters[i].index > index) // not found, this index is not chapter start
            break;
    }
    return true;
}

bool BookBase::isCover(int index) const
{
    return m_cover && index == 0;
}

bool BookBase::hasCover() const
{
    return m_cover != nullptr;
}

const wchar_t* BookBase::fileName() const
{
    return m_fileName.c_str();
}

const wchar_t* BookBase::bookName() const
{
    return m_bookName.c_str();
}

void BookBase::setChapterRule(const ChapterRule* rule)
{
    m_chapterRule = rule;
}

void BookBase::setBookInfo(BookInfo* bookInfo)
{
    m_bookInfo = bookInfo;
}

BookErrno BookBase::open()
{
    auto ret = parseBook();
    if (m_bookInfo && (m_bookInfo->index < 0 || m_bookInfo->index >= m_length)) m_bookInfo->index = 0;
    return ret;
}

void BookBase::close()
{
    m_abort = true;
}

bool BookBase::isAbort() const
{
    return m_abort;
}

int BookBase::index() const
{
    if (m_bookInfo) return m_bookInfo->index;
    return 0;
}

bool BookBase::setIndex(int index)
{
    if (!m_bookInfo) return false;
    if (index >= 0 && index < m_length && index != m_bookInfo->index)
    {
        m_bookInfo->index = index;
        return true;
    }
    return false;
}

int BookBase::curChapterIndex() const
{
    int chapterIdx = 0;
    for (size_t i = 0; i < m_chapters.size(); i++)
    {
        const auto& c = m_chapters[i];
        if (c.index == -1) continue;
        if (c.index > index()) break;
        chapterIdx = (int)i;
    }
    return chapterIdx;
}

int BookBase::chapterCount() const
{
    return (int)m_chapters.size();
}

const Chapter* BookBase::chapter(int chapterIndex) const
{
    if (chapterIndex >= 0 && chapterIndex < (int)m_chapters.size()) return &m_chapters[chapterIndex]; 
    return nullptr;
}

const Cover* BookBase::cover() const
{
    return m_cover;
}

bool BookBase::isEditSupport() const
{
    return false;
}

bool BookBase::editText(int start, int srcLen, wchar_t* const dstText, int dstLen)
{
    return false;
}

bool BookBase::isFetchSupport() const
{
    return false;
}

bool BookBase::fetchChapter(int chapterIndex, AsyncCallback cb)
{
    return false;
}

bool BookBase::isRefreshSupport() const
{
    return false;
}

bool BookBase::refreshChapter(AsyncCallback cb)
{
    return false;
}

bool BookBase::refreshContent(int chapterIndex, AsyncCallback cb)
{
    return false;
}

LRESULT BookBase::onEvent(WPARAM wParam, LPARAM lParam)
{
    return -1;
}

LRESULT BookBase::onTimer(WPARAM wParam, LPARAM lParam)
{
    return -1;
}

bool BookBase::decodeText(const char* src, int slen, wchar_t** dst, int* dlen)
{
    wchar_t* utf16 = nullptr;
    int utf16Len = conv_to_utf16(src, slen, &utf16);
    if (!utf16)
    {
        return false;
    }

    *dlen = utf16Len;
    *dst = (wchar_t*)malloc((utf16Len + 1) * sizeof(wchar_t));
    if (!(*dst))
    {
        conv_free(utf16);
        *dst = nullptr;
        *dlen = 0;
        lgerror("Failed to allocate memory.");
        return false;
    }
    memcpy(*dst, utf16, utf16Len * sizeof(wchar_t));
    (*dst)[*dlen] = 0;
    conv_free(utf16);

    if (!formatText(*dst, dlen))
    {
        free(*dst);
        *dst = nullptr;
        *dlen = 0;
        lgerror("Failed to format book text.");
        return false;
    }
    return true;
}

bool BookBase::formatText(wchar_t* p_data, int* p_len)
{
    if (!p_data || !p_len || *p_len <= 0) return false;

    wchar_t* p_src_text = p_data;
    int src_len = *p_len;
    int dst_len = 0;

    int line_len = 0, crlf_len = 0, prefix_blank_len = 0, suffix_blank_len = 0;
    bool is_blank_line = false;
    bool is_changed = false;

    while (Utils::nextLine(p_src_text, src_len - (int)(p_src_text - p_data), &line_len, &crlf_len, &is_blank_line, &prefix_blank_len, &suffix_blank_len))
    {
        if (is_blank_line)
        {
            p_src_text += line_len + crlf_len; // CRLF
            is_changed = true;
            continue;
        }

        int effective_len = line_len - prefix_blank_len - suffix_blank_len;
        if (effective_len > 0)
        {
            if (prefix_blank_len >= 2)
            {
                p_data[dst_len++] = 0x3000;
                p_data[dst_len++] = 0x3000;

                memmove(p_data + dst_len, p_src_text + prefix_blank_len, sizeof(wchar_t) * effective_len);
                dst_len += effective_len;
                is_changed = true;
            }
            else if (prefix_blank_len == 1 && p_src_text[0] != L'\t')
            {
                p_data[dst_len++] = L'\t';
                memmove(p_data + dst_len, p_src_text + prefix_blank_len, sizeof(wchar_t) * effective_len);
                dst_len += effective_len;
                is_changed = true;
            }
            else
            {
                memmove(p_data + dst_len, p_src_text, sizeof(wchar_t) * (line_len - suffix_blank_len));
                dst_len += line_len - suffix_blank_len;
                if (suffix_blank_len > 0) is_changed = true;
            }
        }

        if (crlf_len > 0) // add \n
        {
            if (crlf_len > 1 || p_src_text[line_len] != 0x0A) is_changed = true;
            p_data[dst_len++] = 0x0A;
        }

        p_src_text += line_len + crlf_len; // CRLF
    }

    if (is_changed)
    {
        p_data[dst_len] = 0;
        *p_len = dst_len;
    }

    return true;
}

bool BookBase::formatTitle(wchar_t* text, int* length)
{
    if (!text || !length || *length <= 0)
    {
        if (length) *length = 0;
        return true;
    }

    int src = 0;
    int dst = 0;
    int len = *length;
    bool in_whitespace = false;

    // Trim Left
    while (src < len && (Utils::isBlank(text[src]) || Utils::isCrLf(text[src])))
    {
        src++;
    }

    while (src < len)
    {
        wchar_t ch = text[src++];
        if ((Utils::isBlank(ch) || Utils::isCrLf(ch)))
        {
            in_whitespace = true;
        }
        else
        {
            if (in_whitespace)
            {
                text[dst++] = L' ';
                in_whitespace = false;
            }
            text[dst++] = ch;
        }
    }

    text[dst] = 0;
    *length = dst;
    return true;
}

void BookBase::freeCover()
{
    if (m_cover)
    {
        if (m_cover->data)
        {
            free(m_cover->data);
            m_cover->data = nullptr;
        }
        free(m_cover);
        m_cover = nullptr;
    }
}
