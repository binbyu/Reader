#include "Book.h"
#include "types.h"
#include "Utils.h"
#include <process.h>
#ifdef _DEBUG
#include <assert.h>
#endif

#define MAX_BLANK_LINE      2

Book::Book()
    : m_Data(NULL)
    , m_Size(0)
    , m_hThread(NULL)
    , m_bForceKill(FALSE)
    , m_Rule(NULL)
{
    memset(m_fileName, 0, sizeof(m_fileName));
    m_Chapters.clear();
}

Book::~Book()
{
    ForceKill();
    CloseBook();
}

BOOL Book::OpenBook(HWND hWnd)
{
    unsigned threadID;
    ob_thread_param_t *param;

    ForceKill();
    param = (ob_thread_param_t *)malloc(sizeof(ob_thread_param_t));
    param->_this = this;
    param->hWnd = hWnd;
    m_hThread = (HANDLE)_beginthreadex(NULL, 0, OpenBookThread, param, 0, &threadID);
    return TRUE;
}

BOOL Book::OpenBook(char *data, int size, HWND hWnd)
{
    unsigned threadID;
    ob_thread_param_t *param;

    m_Data = data;
    m_Size = size;

    ForceKill();
    param = (ob_thread_param_t *)malloc(sizeof(ob_thread_param_t));
    param->_this = this;
    param->hWnd = hWnd;
    m_hThread = (HANDLE)_beginthreadex(NULL, 0, OpenBookThread, param, 0, &threadID);
    return TRUE;
}

BOOL Book::CloseBook(void)
{
    if (m_Text)
    {
        free(m_Text);
        m_Text = NULL;
    }
    m_Length = 0;
    m_Chapters.clear();
    memset(m_fileName, 0, sizeof(m_fileName));
    if (m_Data)
    {
        free(m_Data);
        m_Data = NULL;
    }
    m_Size = 0;
    return TRUE;
}

BOOL Book::IsLoading(void)
{
    return m_hThread != NULL;
}

wchar_t * Book::GetText(void)
{
    return m_Text;
}

void Book::SetFileName(const TCHAR *fileName)
{
    _tcscpy(m_fileName, fileName);
}

TCHAR * Book::GetFileName(void)
{
    return m_fileName;
}

int Book::GetTextLength(void)
{
    return m_Length;
}

chapters_t * Book::GetChapters(void)
{
    return &m_Chapters;
}

void Book::SetChapterRule(chapter_rule_t *rule)
{
    m_Rule = rule;
}

void Book::JumpChapter(HWND hWnd, int index)
{
    if (IsValid())
    {
        if (index >= 0 && index < (int)m_Chapters.size())
        {
            m_Index = m_Chapters[index].index;
            ReDraw(hWnd);
        }
    }
}

void Book::JumpPrevChapter(HWND hWnd)
{
    chapters_t::reverse_iterator itor;

    if (IsValid() && !IsFirstPage())
    {
        for (itor = m_Chapters.rbegin(); itor != m_Chapters.rend(); itor++)
        {
            if (itor->index < m_Index)
            {
                m_Index = itor->index;
                ReDraw(hWnd);
                break;
            }
        }
    }
}

void Book::JumpNextChapter(HWND hWnd)
{
    chapters_t::iterator itor;

    if (IsValid() && !IsLastPage())
    {
        for (itor = m_Chapters.begin(); itor != m_Chapters.end(); itor++)
        {
            if (itor->index > m_Index)
            {
                m_Index = itor->index;
                ReDraw(hWnd);
                break;
            }
        }
    }
}

int Book::GetCurChapterIndex(void)
{
    int index = -1;
    int i;

    if (IsValid())
    {
        index = 0;
        for (i = 0; i < (int)m_Chapters.size(); i++)
        {
            if (m_Chapters[i].index > m_Index)
                break;
            index = i;
        }
    }
    return index;
}

LRESULT Book::OnBookEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

BOOL Book::GetChapterTitle(TCHAR* title, int size)
{
    int index = -1;

    if (IsValid())
    {
        index = GetCurChapterIndex();
        if (index >= 0 && index < (int)m_Chapters.size())
        {
            _tcsncpy(title, m_Chapters[index].title.c_str(), size-1);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL Book::DecodeText(const char *src, int srcsize, wchar_t **dst, int *dstsize)
{
    type_t bom = Unknown;

    if (Unknown != (bom = check_bom(src, srcsize)))
    {
        if (utf8 == bom)
        {
            src += 3;
            srcsize -= 3;
            *dst = utf8_to_utf16(src, srcsize, dstsize);
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
            *dst = (wchar_t *)be_to_le((char *)(*dst), srcsize);
        }
        else if (utf32_le == bom || utf32_be == bom)
        {
            // not support
            return FALSE;
        }
    }
    else if (is_utf8(src, srcsize > 4096 ? 4096 : srcsize))
    {
        *dst = utf8_to_utf16(src, srcsize, dstsize);
    }
    else
    {
        *dst = ansi_to_utf16(src, srcsize, dstsize);
    }

    FormatText(*dst, dstsize);

    return TRUE;
}

BOOL Book::IsChapterIndex(int index)
{
    chapters_t::iterator it;
    for (it = m_Chapters.begin(); it != m_Chapters.end(); it++)
    {
        if (index == it->index)
            return TRUE;
    }
    return FALSE;
}

BOOL Book::IsChapter(int index)
{
    chapters_t::iterator it;
    for (it = m_Chapters.begin(); it != m_Chapters.end(); it++)
    {
        if (index >= it->index && index < it->index + (int)it->title_len /*&& m_Text[index] == it->title[index-it->index]*/)
            return TRUE;
    }
    return FALSE;
}

BOOL Book::GetChapterInfo(int type, int *start, int *length)
{
    int index;
    *start = 0;
    *length = 0;

    if (m_Chapters.empty())
        return FALSE;
    
    index = GetCurChapterIndex();
    if (index == -1)
        return FALSE;

    if (type == -1) // prev chapter
    {
        if (index > 0)
            index--;
        else
            return FALSE;
    }
    else if (type == 1) // next chapter
    {
        if (index + 1 < (int)m_Chapters.size())
            index++;
        else
            return FALSE;
    }
    else // curn chapter
    {
    }
    
    if (index == 0) // first chapter
    {
        *start = GetTextBeginIndex();
    }
    else
    {
        *start = m_Chapters[index].index;
    }

    if (index + 1 == (int)m_Chapters.size()) // last chapter
    {
        *length = m_Length - (*start);
    }
    else
    {
        *length = m_Chapters[index+1].index - (*start);
    }
    return TRUE;
}

BOOL Book::IsValid(void)
{
    return Page::IsValid() && !IsLoading();
}

BOOL Book::FormatText(wchar_t *p_data, int *p_len)
{
    wchar_t *p_src_text = p_data, *p_dst_text;
    int src_len = *p_len, dst_len = 0;
    int line_len = 0, lf_len = 0, is_blank_line = 0, prefix_blank_len = 0, suffix_blank_len = 0;
    int blank_line_num = 0;
    int is_first_line = TRUE;

    if (!p_src_text || src_len <= 0)
        return FALSE;

    p_dst_text = (wchar_t *)malloc(sizeof(wchar_t) * (src_len + 1));
    if (!p_dst_text)
        return FALSE;

    while (GetLine(p_src_text, src_len - (int)(p_src_text - p_data), &line_len, &lf_len, &is_blank_line, &prefix_blank_len, &suffix_blank_len))
    {
        if (is_blank_line)
        {
            if (is_first_line || ++blank_line_num >= MAX_BLANK_LINE)
            {
                p_src_text += line_len + lf_len; // CRLF
                continue;
            }
        }
        else
        {
            blank_line_num = 0;
        }
        is_first_line = FALSE;

        if (line_len - prefix_blank_len - suffix_blank_len > 0)
        {
            // Remove extra prefix spaces
            if (prefix_blank_len > 4
                && (p_src_text[0] == 0x20 || p_src_text[0] == 0xA0)
                /*&& (p_src_text[1] == 0x20 || p_src_text[1] == 0xA0)
                && (p_src_text[2] == 0x20 || p_src_text[2] == 0xA0)
                && (p_src_text[3] == 0x20 || p_src_text[3] == 0xA0)*/)
            {
                p_dst_text[dst_len++] = 0x20;
                p_dst_text[dst_len++] = 0x20;
                p_dst_text[dst_len++] = 0x20;
                p_dst_text[dst_len++] = 0x20;
                memcpy(p_dst_text + dst_len, p_src_text + prefix_blank_len, sizeof(wchar_t) * (line_len - prefix_blank_len - suffix_blank_len));
                dst_len += line_len - prefix_blank_len - suffix_blank_len;
            }
            // Remove extra prefix spaces
            else if (prefix_blank_len > 2
                && p_src_text[0] == 0x3000
                /*&& p_src_text[1] == 0x3000*/)
            {
                p_dst_text[dst_len++] = 0x3000;
                p_dst_text[dst_len++] = 0x3000;
                memcpy(p_dst_text + dst_len, p_src_text + prefix_blank_len, sizeof(wchar_t) * (line_len - prefix_blank_len - suffix_blank_len));
                dst_len += line_len - prefix_blank_len - suffix_blank_len;
            }
            else
            {
                memcpy(p_dst_text + dst_len, p_src_text, sizeof(wchar_t) * (line_len - suffix_blank_len));
                dst_len += line_len - suffix_blank_len;
            }
        }
        if (lf_len > 0) // add \n
            p_dst_text[dst_len++] = 0x0A;

        p_src_text += line_len + lf_len; // CRLF
    }

    memcpy(p_data, p_dst_text, sizeof(wchar_t) * dst_len);
    p_data[dst_len] = 0;
    *p_len = dst_len;
    free(p_dst_text);
    return TRUE;
}

BOOL Book::GetLine(wchar_t* text, int len, int *line_len, int *lf_len, int *is_blank_line, int *prefix_blank_len, int *suffix_blank_len)
{
    int i;
    int is_prefix = 1, is_suffix = 0;

    if (line_len)
        *line_len = 0;
    if (lf_len)
        *lf_len = 0;
    if (is_blank_line)
        *is_blank_line = 1;
    if (prefix_blank_len)
        *prefix_blank_len = 0;
    if (suffix_blank_len)
        *suffix_blank_len = 0;

    if (!text || len <= 0)
        return FALSE;

    for (i = 0; i < len; i++)
    {
        if (is_blank(text[i]))
        {
            if (is_prefix && prefix_blank_len)
            {
                (*prefix_blank_len)++;
            }
            if (is_suffix && suffix_blank_len)
            {
                (*suffix_blank_len)++;
            }
        }
        else if (!IsNewLine(text[i]))
        {
            is_prefix = 0;
            is_suffix = 1;
            if (suffix_blank_len)
                *suffix_blank_len = 0;
            if (is_blank_line)
                *is_blank_line = 0;
        }

        if ((i+1) < len && text[i] == 0x0D && text[i+1] == 0x0A) // \r\n
        {
            if (line_len)
                *line_len = i;
            if (lf_len)
                *lf_len = 2;
            return TRUE;
        }

        if (text[i] == 0x0A) // \n
        {
            if (line_len)
                *line_len = i;
            if (lf_len)
                *lf_len = 1;
            return TRUE;
        }
    }
    if (line_len)
        *line_len = len;
    if (lf_len)
        *lf_len = 0;
    return TRUE;
}

void Book::ForceKill(void)
{
    if (m_hThread)
    {
        m_bForceKill = TRUE;
        if (WAIT_TIMEOUT == WaitForSingleObject(m_hThread, 5000))
        {
            ASSERT(FALSE);
            TerminateThread(m_hThread, 0);
        }
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
}

unsigned __stdcall Book::OpenBookThread(void* pArguments)
{
    ob_thread_param_t *param = (ob_thread_param_t *)pArguments;
    Book *_this = param->_this;
    BOOL result = FALSE;

    _this->m_bForceKill = FALSE;
    result = _this->ParserBook(param->hWnd);
    if (param->hWnd && !_this->m_bForceKill)
    {
        PostMessage(param->hWnd, WM_OPEN_BOOK, result ? 1 : 0, NULL);
    }
    free(param);
    CloseHandle(_this->m_hThread);
    _this->m_hThread = NULL;
    _endthreadex(0);
    return 0;
}