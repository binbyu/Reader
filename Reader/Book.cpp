#include "stdafx.h"
#include "Book.h"
#include "types.h"
#include "Utils.h"


Book::Book()
    : m_Data(NULL)
    , m_Size(0)
{
    memset(m_fileName, 0, sizeof(m_fileName));
    m_Chapters.clear();
}

Book::~Book()
{
    CloseBook();
}

bool Book::OpenBook(const TCHAR *fileName)
{
    _tcscpy(m_fileName, fileName);
    return ParserBook();
}

bool Book::OpenBook(char *data, int size)
{
    m_Data = data;
    m_Size = size;
    return ParserBook();
}

bool Book::CloseBook(void)
{
    if (m_Text)
        free(m_Text);
    m_TextLength = 0;
    m_Chapters.clear();
    memset(m_fileName, 0, sizeof(m_fileName));
    if (m_Data)
        free(m_Data);
    m_Size = 0;
    return true;
}

wchar_t * Book::GetText(void)
{
    return m_Text;
}

int Book::GetTextLength(void)
{
    return m_TextLength;
}

chapters_t * Book::GetChapters(void)
{
    return &m_Chapters;
}

void Book::JumpChapter(HWND hWnd, int index)
{
    if (!m_Text)
        return;

    if (m_Chapters.end() != m_Chapters.find(index))
    {
        (*m_CurrentPos) = m_Chapters[index].index;
        Reset(hWnd);
    }
}

void Book::JumpPrevChapter(HWND hWnd)
{
    chapters_t::reverse_iterator itor;

    if (!m_Text || IsFirstPage())
        return;

    for (itor = m_Chapters.rbegin(); itor != m_Chapters.rend(); itor++)
    {
        if (itor->second.index < (*m_CurrentPos))
        {
            (*m_CurrentPos) = itor->second.index;
            Reset(hWnd);
            break;
        }
    }
}

void Book::JumpNextChapter(HWND hWnd)
{
    chapters_t::iterator itor;

    if (!m_Text || IsLastPage())
        return;

    for (itor = m_Chapters.begin(); itor != m_Chapters.end(); itor++)
    {
        if (itor->second.index > (*m_CurrentPos))
        {
            (*m_CurrentPos) = itor->second.index;
            Reset(hWnd);
            break;
        }
    }
}

bool Book::DecodeText(const char *src, int srcsize, wchar_t **dst, int *dstsize)
{
    type_t bom = Unknown;

    if (Unknown != (bom = Utils::check_bom(src, srcsize)))
    {
        if (utf8 == bom)
        {
            src += 3;
            srcsize -= 3;
            *dst = Utils::utf8_to_utf16(src, dstsize);
            (*dstsize)--;
        }
        else if (utf16_le == bom)
        {
            src += 2;
            srcsize -= 2;
            *dstsize = srcsize / 2;
            *dst = (wchar_t *)malloc(sizeof(wchar_t) * ((*dstsize) + 1));
            memcpy(*dst, src, srcsize);
            (*dst)[*dstsize] = 0;
        }
        else if (utf16_be == bom)
        {
            src += 2;
            srcsize -= 2;
            *dstsize = srcsize / 2;
            *dst = (wchar_t *)malloc(sizeof(wchar_t) * ((*dstsize) + 1));
            memcpy(*dst, src, srcsize);
            (*dst)[*dstsize] = 0;
            *dst = (wchar_t *)Utils::be_to_le((char *)(*dst), srcsize);
        }
        else if (utf32_le == bom || utf32_be == bom)
        {
            // not support
            return false;
        }
    }
    else if (Utils::is_ascii(src, srcsize > 1024 ? 1024 : srcsize))
    {
        *dst = Utils::utf8_to_utf16(src, dstsize);
        (*dstsize)--;
    }
    else if (Utils::is_utf8(src, srcsize > 1024 ? 1024 : srcsize))
    {
        *dst = Utils::utf8_to_utf16(src, dstsize);
        (*dstsize)--;
    }
    else
    {
        *dst = Utils::ansi_to_utf16(src, dstsize);
        (*dstsize)--;
    }

    FormatText(*dst, dstsize);

    return true;
}

bool Book::FormatText(wchar_t *text, int *len)
{
    wchar_t *buf = NULL;
    bool flag = true;
    int i, index = 0;
    if (!text || *len == 0)
        return false;

    buf = (wchar_t *)malloc(((*len) + 1) * sizeof(wchar_t));
    for (i = 0; i < (*len); i++)
    {
        if (flag && IsBlanks(text[i]))
            continue;
        flag = false;
        if (text[i] == 0x0D && text[i + 1] == 0x0A)
            i++;
        buf[index++] = text[i];
    }
    buf[index] = 0;
    *len = index;
    memcpy(text, buf, ((*len) + 1) * sizeof(wchar_t));
    free(buf);
    return true;
}

bool Book::IsBlanks(wchar_t c)
{
    if (/*c == 0x20 ||*/ c == 0x09 || c == 0x0A || c == 0x0D)
    {
        return true;
    }
    return false;
}

bool Book::CalcMd5(TCHAR *fileName, u128_t *md5, char **data, int *size)
{
    FILE *fp = NULL;
    int _size;
    char *_data;
    
    fp = _tfopen(fileName, _T("rb"));
    if (!fp)
        return false;
    fseek(fp, 0, SEEK_END);
    _size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    _data = (char *)malloc(_size + 1);
    if (!_data)
    {
        fclose(fp);
        return false;
    }
    if (fread(_data, 1, _size, fp) != _size)
    {
        free(_data);
        fclose(fp);
        return false;
    }
    _data[_size] = 0;
    fclose(fp);

    if (!Utils::get_md5(_data, _size, md5))
    {
        free(_data);
        return false;
    }
    *data = _data;
    *size = _size;
    return true;
}
