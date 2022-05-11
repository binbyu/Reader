#include "Page.h"
#include "Book.h"

#define CHAR_GAP                (m_header->char_gap)
#define LINE_GAP                (m_header->line_gap)
#define PART_GAP                (m_header->paragraph_gap)
#define LEFT_MIN                (m_header->internal_border.left)
#define TOP_MIN                 (m_header->internal_border.top)
#define RIGHT_MIN               (m_header->internal_border.right)
#define BOTTOM_MIN              (m_header->internal_border.bottom)
#define NO_BLANS_LINE           (m_header->blank_lines)
#define LINE_INDENT             (m_header->line_indent)
#define WORD_WRAP               (m_header->word_wrap)
#define CHAPTER_PAGE            (m_header->chapter_page)
#define LINE_NUM                (m_header->wheel_speed)
#define LEFT_NUM                (m_header->left_line_count)
#if ENABLE_TAG
#define TAGS                    (m_header->tags)
#define TAG_COUNT               (m_header->tag_count)
#endif

extern VOID Invalidate(HWND, BOOL, BOOL);
extern void Save(HWND hWnd);

Page::Page()
    : m_Text(NULL)
    , m_Length(0)
    , m_pIndex(NULL)
    , m_PageLength(0)
    , m_header(0)
    , m_dcList(NULL)
    , m_dcIndex(0)
    , m_LineCount(0)
    , m_DrawType(DRAW_NULL)
    , m_BlankPage(TRUE)
    , m_ChapterStart(0)
    , m_ChapterLength(0)
{
    memset(&m_PageInfo, 0, sizeof(page_info_t));
}

Page::~Page()
{
    ReleasePageInfo();
}

void Page::Init(int *p_index, header_t *header)
{
    m_pIndex = p_index;
    m_header = header;
    if (m_Index <= 0 || m_Index > m_Length)
        m_Index = 0;
}

void Page::PageUp(HWND hWnd, BOOL draw)
{
    if (!OnUpDownEvent(hWnd, DRAW_PAGE_UP))
        return;

    if (draw && !IsValid())
        return;

    if (m_Index > 0)
    {
        m_DrawType = DRAW_PAGE_UP;
        m_LineCount = LEFT_NUM;
        if (draw)
            ReDraw(hWnd);
    }
}

void Page::PageDown(HWND hWnd, BOOL draw)
{
    if (!OnUpDownEvent(hWnd, DRAW_PAGE_DOWN))
        return;

    if (draw && !IsValid())
        return;

    if (m_Index + m_PageInfo.length < m_Length)
    {
        m_DrawType = DRAW_PAGE_DOWN;
        m_LineCount = LEFT_NUM;
        if (draw)
            ReDraw(hWnd);
    }
}

void Page::LineUp(HWND hWnd, BOOL draw)
{
    if (!OnUpDownEvent(hWnd, DRAW_LINE_UP))
        return;

    if (draw && !IsValid())
        return;

    if (m_Index > 0)
    {
        switch (LINE_NUM)
        {
        case ws_single_line:
            m_LineCount = 1;
            break;
        case ws_double_line:
            m_LineCount = 2;
            break;
        case ws_three_line:
            m_LineCount = 3;
            break;
        default:
            return PageUp(hWnd);
            break;
        }
        m_DrawType = DRAW_LINE_UP;
        if (draw)
            ReDraw(hWnd);
    }
}

void Page::LineDown(HWND hWnd, BOOL draw)
{
    if (!OnUpDownEvent(hWnd, DRAW_LINE_DOWN))
        return;

    if (draw && !IsValid())
        return;

    if (m_Index + m_PageInfo.length < m_Length)
    {
        switch (LINE_NUM)
        {
        case ws_single_line:
            m_LineCount = 1;
            break;
        case ws_double_line:
            m_LineCount = 2;
            break;
        case ws_three_line:
            m_LineCount = 3;
            break;
        default:
            return PageDown(hWnd);
            break;
        }
        m_DrawType = DRAW_LINE_DOWN;
        if (draw)
            ReDraw(hWnd);
    }
}

void Page::DrawPage(HWND hWnd, HDC hdc, RECT* rc, BOOL enable_alpha)
{
    int i, j, x, y;
    line_info_t* p_line;
    char_info_t* p_char;
    alpha_dc_info_t alpha_dc;

    if (enable_alpha)
        m_BlankPage = TRUE;
    else
        m_BlankPage = FALSE;

    if (!IsValid())
        return;

    if (!OnDrawPageEvent(hWnd))
        return;

    if (enable_alpha)
    {
        memset(&alpha_dc, 0, sizeof(alpha_dc_info_t));
        CreateAlphaTextBitmap(hdc, rc->right - rc->left, rc->bottom - rc->top, &alpha_dc);
    }

    if (DrawCover(hdc, rc))
    {
        m_Index = 0;
        m_PageLength = 1;
        m_DrawType = DRAW_NULL;
        m_LineCount = 0;
        m_PageInfo.start = m_Index;
        m_PageInfo.length = 1;
        ClearLines();
        if (enable_alpha)
        {
            DeleteAlphaTextBitmap(hdc, &alpha_dc);
            m_BlankPage = FALSE;
        }
        return;
    }

    BeginDraw();

    switch (m_DrawType)
    {
    case DRAW_NULL:
    case DRAW_PAGE_DOWN:
    case DRAW_LINE_DOWN:
        CalcPageDown(hdc, rc);
        break;
    case DRAW_PAGE_UP:
    case DRAW_LINE_UP:
        CalcPageUp(hdc, rc);
        break;
    default:
        break;
    }

    m_DrawType = DRAW_NULL;
    m_LineCount = 0;

    y = TOP_MIN;
    for (i = 0; i < m_PageInfo.lines.used; i++)
    {
        p_line = &m_PageInfo.lines.lines[i];
        x = LEFT_MIN + p_line->x;
        for (j = 0; j < p_line->char_cnt; j++)
        {
            p_char = &p_line->chars[j];
            if (enable_alpha)
            {
                DrawAlphaText(hdc, p_char, x, y, p_line->cy, &alpha_dc);
            }
            else
            {
                SelectFontByDcIndex(hdc, p_char->dc_idx);
                TextOut(hdc, x, y + (p_line->cy - p_char->cy), &m_Text[p_char->idx], 1);
            }
            x += p_char->cx + CHAR_GAP;
        }
        y += p_line->cy + p_line->gap;
    }

    m_Index = m_PageInfo.start;
    m_PageLength = m_PageInfo.length;

    if (enable_alpha)
    {
        DeleteAlphaTextBitmap(hdc, &alpha_dc);
    }
    EndDraw();
    Save(hWnd);
    TestCase(rc);
}

void Page::ReDraw(HWND hWnd)
{
    Invalidate(hWnd, TRUE, FALSE);
}

int Page::GetPageLength(void)
{
    return m_PageLength;
}

int Page::GetTextLength(void)
{
    return m_Length;
}

BOOL Page::IsFirstPage(void)
{
    return m_pIndex && m_Index == 0;
}

BOOL Page::IsLastPage(void)
{
    return m_pIndex && (m_Index + m_PageLength == m_Length);
}

BOOL Page::IsCoverPage(void)
{
    return m_pIndex && m_Index == 0 && GetCover();
}

double Page::GetProgress(void)
{
    return m_Length == 0 ? 0.0f : ((m_Index + m_PageLength) * 100.0f / m_Length);
}

BOOL Page::GetCurPageText(TCHAR **text)
{
    int newlinecount = 0;
    TCHAR *c = NULL;
    int i,j;

    if (m_PageLength > 0)
    {
        for (i=0; i<m_PageLength; i++)
        {
            c = m_Text + (m_Index + i);
            if (*c == 0x0A)
            {
                newlinecount++;
            }
        }

        *text = (TCHAR *)malloc(sizeof(TCHAR) * (m_PageLength + newlinecount + 1));

        for (i=0,j=0; i<m_PageLength; i++)
        {
            c = m_Text + (m_Index + i);
            if (*c == 0x0A)
            {
                (*text)[j++] = 0x0D;
            }
            (*text)[j++] = *c;
        }
        (*text)[j] = 0;
        return TRUE;
    }
    return FALSE;
}

BOOL Page::SetCurPageText(HWND hWnd, TCHAR *dst_text)
{
    Book *book = NULL;
    TCHAR *src_text = NULL;
    TCHAR *text = NULL;
    int dst_len;
    int src_len;
    int len;

    book = dynamic_cast<Book *>(this);
    if (!book)
        return FALSE;

    if (GetCurPageText(&src_text))
    {
        if (0 == _tcscmp(dst_text, src_text))
        {
            return TRUE;
        }

        // format dest text
        src_len = m_PageLength;
        dst_len = (int)_tcslen(dst_text);
        book->FormatText(dst_text, &dst_len);

        // change text
        len = m_Length - src_len + dst_len;
        text = (TCHAR *)malloc(sizeof(TCHAR) * (len+1));
        text[len] = 0;
        if (m_Index > 0)
            memcpy(text, m_Text, sizeof(TCHAR) * m_Index);
        memcpy(text+m_Index, dst_text, sizeof(TCHAR) * dst_len);
        memcpy(text+m_Index+dst_len, m_Text+m_Index+m_PageLength, sizeof(TCHAR) * (m_Length-m_Index-m_PageLength));
        free(m_Text);
        m_Text = text;
        m_Length = len;

        free(src_text);

        // save file
        if (!book->SaveBook(NULL))
            return FALSE;

        // update chapter
        if (!book->UpdateChapters(dst_len - src_len))
            return FALSE;

        // redraw page
        ReDraw(hWnd);
        return TRUE;
    }
    return FALSE;
}

BOOL Page::IsBlankPage(void)
{
    return m_BlankPage;
}

BOOL Page::DrawCover(HDC hdc, RECT* rc)
{
    Gdiplus::Bitmap *cover = NULL;
    Gdiplus::Graphics *g = NULL;
    int w,h,bw,bh;
    double d,bd;
    Gdiplus::Rect src;
    Gdiplus::Rect dst;

    cover = GetCover();
    if (!cover)
        return FALSE;

    switch (m_DrawType)
    {
    case DRAW_NULL:
        if (m_Index != 0)
            return FALSE;
        break;
    case DRAW_PAGE_UP:
    case DRAW_LINE_UP:
        if (m_Index != GetTextBeginIndex())
            return FALSE;
        break;
    default:
        return FALSE;
        break;
    }

    // calc image rect
    w = rc->right - rc->left;
    h = rc->bottom - rc->top;
    bw = cover->GetWidth();
    bh = cover->GetHeight();
    d = ((double)w)/h;
    bd = ((double)bw)/bh;
    if (bd > d)
    {
        // image is too wide
        bw = w;
        bh = (int)(bw / bd);
    }
    else if (bd < d)
    {
        // image is too high
        bh = h;
        bw = (int)(bd * bh);
    }
    src.X = 0;
    src.Y = 0;
    src.Width = cover->GetWidth();
    src.Height = cover->GetHeight();
    dst.X = (w - bw)/2;
    dst.Y = (h - bh)/2;
    dst.Width = bw;
    dst.Height = bh;
    g = new Gdiplus::Graphics(hdc);
    g->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g->DrawImage(cover, dst, src.X, src.Y, src.Width, src.Height, Gdiplus::UnitPixel);
    delete g;
    return TRUE;
}

void Page::CreateAlphaTextBitmap(HDC hdc, int width, int height, alpha_dc_info_t *p_alpha_dc)
{
    BITMAPINFOHEADER BMIH;

    // Specify DIB setup
    memset(&BMIH, 0x0, sizeof(BITMAPINFOHEADER));
    BMIH.biSize = sizeof(BMIH);
    BMIH.biWidth = width;
    BMIH.biHeight = height;
    BMIH.biPlanes = 1;
    BMIH.biBitCount = 32;
    BMIH.biCompression = BI_RGB;

    // Create and select DIB into DC
    p_alpha_dc->width = width;
    p_alpha_dc->height = height;
    p_alpha_dc->hDIB = CreateDIBSection(hdc, (LPBITMAPINFO)&BMIH, 0, (LPVOID*)&p_alpha_dc->pvBits, NULL, 0); 
    SelectObject(hdc, p_alpha_dc->hDIB);
}

void Page::DeleteAlphaTextBitmap(HDC hdc, alpha_dc_info_t *p_alpha_dc)
{
    if (p_alpha_dc)
    {
        if (p_alpha_dc->hDIB)
            DeleteObject(p_alpha_dc->hDIB);
    }
}

void Page::DrawAlphaText(HDC hdc, char_info_t* p_char, int x, int y, int h, alpha_dc_info_t *p_alpha_dc)
{
    BYTE FillR,FillG,FillB,ThisA;
    BYTE *DataPtr;
    BOOL is_tag = FALSE;
    int i,j;
    extern BYTE _textAlpha;

    is_tag = p_char->dc_idx >= 2;

    SelectFontByDcIndex(hdc, p_char->dc_idx);

    if (!is_tag)
    {
        SetTextColor(hdc, 0x00FFFFFF);
        SetBkColor(hdc, 0x00000000);
        SetBkMode(hdc, OPAQUE);

        FillR = GetRValue(m_dcList[p_char->dc_idx].TextColor);
        FillG = GetGValue(m_dcList[p_char->dc_idx].TextColor);
        FillB = GetBValue(m_dcList[p_char->dc_idx].TextColor);
    }

    // draw text
    TextOut(hdc, x, y + (h - p_char->cy), &m_Text[p_char->idx], 1);

    // convert pixel
    for (i = p_alpha_dc->height - y - (h - p_char->cy) - 1; i >= p_alpha_dc->height - y - h; i--)
    {
        for (j = x; j < x + p_char->cx; j++)
        {
            DataPtr = &p_alpha_dc->pvBits[(i * p_alpha_dc->width + j) * 4];

            if (is_tag)
            {
                if (j == x + p_char->cx - 1)
                    continue; // OPAQUE is paints background starting at x-1, ending at cx+1

                if (m_BlankPage)
                    m_BlankPage = FALSE;
                *(DataPtr+3) = 0xff;
            }
            else
            {
                if (m_BlankPage)
                {
                    if ((*((UINT*)DataPtr)) & 0x00FFFFFF)
                        m_BlankPage = FALSE;
                }

                ThisA = *DataPtr; // Move alpha and pre-multiply with RGB 
                *DataPtr++ = (FillB * ThisA * _textAlpha) >> 16; 
                *DataPtr++ = (FillG * ThisA * _textAlpha) >> 16; 
                *DataPtr++ = (FillR * ThisA * _textAlpha) >> 16;
                *DataPtr++ = (ThisA * _textAlpha) >> 8; // Set text Alpha
            }
        }
    }
}

void Page::BeginDraw(void)
{
#if ENABLE_TAG
    int i;
    m_dcList = (dc_info_t*)malloc(sizeof(dc_info_t) * (TAG_COUNT + 2));
#else
    m_dcList = (dc_info_t*)malloc(sizeof(dc_info_t) * 2);
#endif
    m_dcList[0].hFont = CreateFontIndirect(&m_header->font);
    m_dcList[0].BkColor = 0x0;
    m_dcList[0].TextColor = GetTextAlpha(m_header->font_color);
    m_dcList[0].BkMode = TRANSPARENT;
    m_dcList[1].hFont = CreateFontIndirect(&m_header->font_title);
    m_dcList[1].BkColor = 0x0;
    m_dcList[1].TextColor = GetTextAlpha(m_header->font_color_title);
    m_dcList[1].BkMode = TRANSPARENT;
#if ENABLE_TAG
    for (i = 0; i < TAG_COUNT; i++)
    {
        m_dcList[i + 2].hFont = CreateFontIndirect(&TAGS[i].font);
        m_dcList[i + 2].BkColor = TAGS[i].bg_color;
        m_dcList[i + 2].TextColor = TAGS[i].font_color;
        m_dcList[i + 2].BkMode = OPAQUE;
    }
#endif
    m_dcIndex = -1;
}

void Page::EndDraw(void)
{
    int i;
#if ENABLE_TAG
    for (i = 0; i < TAG_COUNT + 2; i++)
#else
    for (i = 0; i < 2; i++)
#endif
    {
        DeleteObject(m_dcList[i].hFont);
    }
    free(m_dcList);
    m_dcList = NULL;
}

DWORD Page::GetTextAlpha(DWORD color)
{
#if 0
    extern BYTE _textAlpha;
    BYTE r, g, b;
    r = (GetRValue(color) * _textAlpha) >> 8;
    g = (GetGValue(color) * _textAlpha) >> 8;
    b = (GetBValue(color) * _textAlpha) >> 8;
    return RGB(r, g, b);
#else
    return color;
#endif
}

int Page::SelectFont(HDC hdc, int index, BOOL is_title)
{
    int dc_index;

#if ENABLE_TAG
    dc_index = IsTag(index);
    if (dc_index >= 0 && dc_index < TAG_COUNT)
    {
        dc_index += 2;
        goto _completed;
    }
#endif
    
    if (m_header->use_same_font)
    {
        dc_index = 0;
        goto _completed;
    }

    if (is_title /*&& IsChapter(index)*/)
    {
        dc_index = 1;
    }
    else
    {
        dc_index = 0;
    }

_completed:
    if (m_dcIndex != dc_index)
    {
        m_dcIndex = dc_index;
        SelectObject(hdc, m_dcList[m_dcIndex].hFont);
        SetBkColor(hdc, m_dcList[m_dcIndex].BkColor);
        SetTextColor(hdc, m_dcList[m_dcIndex].TextColor);
        SetBkMode(hdc, m_dcList[m_dcIndex].BkMode);
    }
    return m_dcIndex;
}

void Page::SelectFontByDcIndex(HDC hdc, int dc_idx)
{
#if ENABLE_TAG
    if ((dc_idx >= 0 && dc_idx < TAG_COUNT+2) && m_dcIndex != dc_idx)
#else
    if ((dc_idx >= 0 && dc_idx < 2) && m_dcIndex != dc_idx)
#endif
    {
        m_dcIndex = dc_idx;
        SelectObject(hdc, m_dcList[m_dcIndex].hFont);
        SetBkColor(hdc, m_dcList[m_dcIndex].BkColor);
        SetTextColor(hdc, m_dcList[m_dcIndex].TextColor);
        SetBkMode(hdc, m_dcList[m_dcIndex].BkMode);
    }
}

int Page::GetPrevParagraph(int start, int max_len, int *is_blank, int *crlf_len)
{
    int i, length = 0;
    int end = start - max_len + 1;

    if (start < 0)
        return length;
    if (end < GetTextBeginIndex())
        end = GetTextBeginIndex();

    if (CHAPTER_PAGE)
    {
        if (end < m_ChapterStart)
            end = m_ChapterStart;
        if (start > m_ChapterStart + m_ChapterLength - 1)
            start = m_ChapterStart + m_ChapterLength - 1;
    }

    *is_blank = 1;
    *crlf_len = 0;
    for (i = start; i >= end; i--, length++)
    {
        if ((i - 1 >= 0 && m_Text[i] == L'\n' && m_Text[i - 1] == L'\r') // \r\n
            || m_Text[i] == L'\r'
            || m_Text[i] == L'\n')
        {
            if (length == 0)
            {
                if (i - 1 >= 0 && m_Text[i] == L'\n' && m_Text[i - 1] == L'\r')
                {
                    i--;
                    length++;
                    *crlf_len += 1;
                }
                *crlf_len += 1;
                continue;
            }
            break;
        }
        if ((*is_blank) && !is_blank(m_Text[i]))
        {
            *is_blank = 0;
        }
    }
    return length;
}

int Page::GetNextParagraph(int start, int max_len, int *is_blank, int *crlf_len)
{
    int i, length = 0;
    int end = start + max_len;

    if (start < 0)
        return length;
    if (end > m_Length)
        end = m_Length;

    *is_blank = 1;
    *crlf_len = 0;

    if (CHAPTER_PAGE)
    {
        if (start < m_ChapterStart)
            start = m_ChapterStart;
        if (end > m_ChapterStart + m_ChapterLength)
            end = m_ChapterStart + m_ChapterLength;
    }

    for (i = start; i < end; i++, length++)
    {
        if ((i + 1 < end && m_Text[i] == L'\r' && m_Text[i + 1] == L'\n') // \r\n
            || m_Text[i] == L'\r'
            || m_Text[i] == L'\n')
        {
            if (i + 1 < end && m_Text[i] == L'\r' && m_Text[i + 1] == L'\n')
            {
                i++;
                length++;
                *crlf_len += 1;
            }
            length++;
            *crlf_len += 1;
            break;
        }
        if ((*is_blank) && !is_blank(m_Text[i]))
        {
            *is_blank = 0;
        }
    }
    return length;
}

int Page::ParagraphToLines(HDC hdc, int start, int length, int width, int height, int line_idx)
{
    int i, j, end = start + length;
    int line_idx_bak = line_idx;
    int is_blank_line;
    int is_new_paragraph = start == 0 ? TRUE : (m_Text[start - 1] == 0x0A ? TRUE : FALSE);
    int is_title = IsChapter(start);
    int indent_width = GetIndentWidth(hdc);
    SIZE sz;
    int x, y, w, h;
    int char_start, line_start, word_start;
    int line_len, char_len, word_height, word_width; // for WORD_WRAP
    char_info_t *chars = (char_info_t *)malloc(sizeof(char_info_t) * length);

    ASSERT(start >= GetTextBeginIndex() && end <= m_Length);
    if (start < 0 || end > m_Length || !chars)
    {
        if (chars)
            free(chars);
        return line_idx - line_idx_bak;
    }

    x = (LINE_INDENT && !is_title && is_new_paragraph) ? indent_width : 0;
    y = 0;
    w = 0;
    h = 0;
    char_start = start;
    word_start = start;
    line_start = start;
    is_blank_line = 1;

    for (i = start; i < end; i++)
    {
        SelectFont(hdc, i, is_title);
        GetTextExtentPoint32(hdc, m_Text + i, 1, &sz);
        chars[i - start].idx = i;
        chars[i - start].dc_idx = m_dcIndex;
        chars[i - start].cx = sz.cx;
        chars[i - start].cy = sz.cy;

        if (i == start && x == indent_width && x + sz.cx > width)
        {
            x = 0;
        }

        if (sz.cx > width)
        {
            break;
        }

        if (y + max(h, sz.cy) > height) // for WORD_WRAP, must using h
        {
            break;
        }

        if (x + w + sz.cx > width)
        {
            if (WORD_WRAP)
            {
                if (is_blank(m_Text[i])) // add left space
                {
                    w += sz.cx + CHAR_GAP;
                    word_start = i+1;
                    continue;
                }
                else
                {
                    if (word_start == char_start) // too long word
                    {
                        // do nothing
                    }
                    else // move current word to next line
                    {
                        line_len = word_start - line_start;
                        char_len = word_start - char_start;
                        word_height = 0;
                        word_width = 0;
                        ASSERT(line_len > 0);
                        ASSERT(char_len >= 0);

                        is_blank_line = 1;
                        word_height = 0;
                        for (j = char_start - start; j < char_start - start + char_len; j++)
                        {
                            word_height = max(word_height, chars[j].cy);
                            if (is_blank_line && !is_blank(m_Text[chars[j].idx]))
                                is_blank_line = 0;
                        }
                        ASSERT(word_height != 0);
                        if (word_height == 0)
                            word_height = h;

                        // add line
                        AddCharsToLine(line_idx++, chars, char_start - start, char_len, line_start, line_len, x, word_height, is_blank_line ? 0 : LINE_GAP);

                        x = 0;
                        y += word_height + (is_blank_line ? 0 : LINE_GAP);
                        w = 0;
                        h = 0;
                        line_start = word_start;
                        char_start = word_start;
                        is_blank_line = 1;
                        for (j = word_start - start; j < i - start; j++)
                        {
                            w += chars[j].cx + CHAR_GAP;
                            h = max(h, chars[j].cy);
                            if (is_blank_line && !is_blank(m_Text[chars[j].idx]))
                                is_blank_line = 0;
                        }
                        ASSERT(word_width <= width);
                        goto _continue;
                    }
                }
            }

            // add line
            ASSERT(h != 0);
            AddCharsToLine(line_idx++, chars, char_start - start, i - char_start, line_start, i - line_start, x, h, is_blank_line ? 0 : LINE_GAP);

            x = 0;
            y += h + (is_blank_line ? 0 : LINE_GAP);
            w = 0;
            h = 0;
            char_start = i;
            word_start = i;
            line_start = i;
            is_blank_line = 1;
        }

    _continue:
        if (is_blank(m_Text[i]))
        {
            // for indent, ignore the blank chars which at the beginning of a paragraph.
            if (LINE_INDENT && char_start == i && !is_title && is_new_paragraph && line_start == start)
            {
                char_start = i + 1;
                word_start = i + 1;
                h = max(h, sz.cy);
                continue;
            }
            word_start = i + 1;
        }
        else if (is_hyphen(m_Text[i]))
        {
            word_start = i + 1;
            is_blank_line = 0;
        }
        else
        {
            is_blank_line = 0;
        }
        w += sz.cx + CHAR_GAP;
        h = max(h, sz.cy);
    }

    h = max(h, sz.cy);
    if (i == end && i - line_start > 0 && y + h <= height)
    {
        // add line
        ASSERT(h != 0);
        AddCharsToLine(line_idx++, chars, char_start - start, i - char_start, line_start, i - line_start, x, h, LINE_GAP);
    }

    free(chars);
    return line_idx - line_idx_bak;
}

int Page::AddCharsToLine(int line_idx, char_info_t *chars, int char_start, int char_len, int line_start, int line_len, int x, int cy, int gap)
{
    const int LINE_UNIT = 32;
    char_info_t *p_chars;
    lines_t *p_lines = &m_PageInfo.lines;

    ASSERT(char_start >= 0);
    ASSERT(line_start >= 0);
    ASSERT(chars ? line_start <= chars[char_start].idx : 1);
    ASSERT(char_len <= line_len);
    ASSERT(cy >= 0);
    ASSERT(line_len > 0);

    if (p_lines->used == p_lines->total)
    {
        p_lines->total += LINE_UNIT;
        p_lines->lines = (line_info_t *)realloc(p_lines->lines, sizeof(line_info_t) * p_lines->total);
    }

    if (WORD_WRAP) // remove left space
    {
        while (char_len > 0 && is_space(m_Text[(chars + char_start + char_len - 1)->idx]))
        {
            char_len--;
        }
    }

    if (char_len > 0)
    {
        p_chars = (char_info_t *)malloc(sizeof(char_info_t) * char_len);
        memcpy(p_chars, chars + char_start, sizeof(char_info_t) * char_len);
    }
    else
    {
        p_chars = NULL;
    }
    

    if (line_idx >= 0 && line_idx < p_lines->used) // insert
    {
        memmove(&p_lines->lines[line_idx + 1], &p_lines->lines[line_idx], sizeof(line_info_t) * (p_lines->used - line_idx));
    }
    else // append
    {
        line_idx = p_lines->used;
    }
    p_lines->lines[line_idx].start = line_start;
    p_lines->lines[line_idx].length = line_len;
    p_lines->lines[line_idx].x = x;
    p_lines->lines[line_idx].cy = cy;
    p_lines->lines[line_idx].gap = gap;
    p_lines->lines[line_idx].chars = p_chars;
    p_lines->lines[line_idx].char_cnt = char_len;
    p_lines->used++;
    return 0;
}

void Page::RemoveLines(int line_idx, int count)
{
    lines_t *p_lines = &m_PageInfo.lines;
    int i, used = p_lines->used;

    ASSERT(line_idx + count <= used);

    for (i = line_idx; i < used && i < line_idx + count; i++)
    {
        if (p_lines->lines[i].chars)
            free(p_lines->lines[i].chars);
        p_lines->used--;
    }

    ASSERT(p_lines->used >= 0);

    if (line_idx + count < used)
    {
        ASSERT(p_lines->used > 0);
        memmove(&p_lines->lines[line_idx], &p_lines->lines[line_idx + count], sizeof(line_info_t) * (used - line_idx - count));
    }
}

void Page::ClearLines(void)
{
    lines_t *p_lines = &m_PageInfo.lines;
    int i;

    for (i = 0; i < p_lines->used; i++)
    {
        if (p_lines->lines[i].chars)
            free(p_lines->lines[i].chars);
    }
    p_lines->used = 0;
}

void Page::ReleasePageInfo(void)
{
    lines_t *p_lines = &m_PageInfo.lines;
    int i;

    for (i = 0; i < p_lines->used; i++)
    {
        if (p_lines->lines[i].chars)
            free(p_lines->lines[i].chars);
    }
    if (p_lines->lines)
        free(p_lines->lines);
    memset(&m_PageInfo, 0, sizeof(page_info_t));
}

void Page::CalcPageDown(HDC hdc, RECT *rc)
{
    int width = rc->right - rc->left - LEFT_MIN - RIGHT_MIN;
    int height = rc->bottom - rc->top - TOP_MIN - BOTTOM_MIN;
    int start_pos, length;
    int is_blank_line = 1;
    int remain_blank_length = 0;
    int crlf_len = 0;
    int line_idx = 0;
    int max_page_length = GetMaxPageLength(hdc, width, height);
    int i, cnt, h, line_cnt;
    SIZE sz;
    line_info_t *p_line;

    h = height;

    if (m_DrawType == DRAW_NULL) // for repaint
    {
        ClearLines();
        start_pos = m_Index;
    }
    else if (m_DrawType == DRAW_PAGE_DOWN)
    {
        // for DRAW_PAGE_DOWN m_LineCount use for 'left lines count'
        if (m_LineCount > 0 && m_PageInfo.lines.used > 0 && !CHAPTER_PAGE)
        {
            cnt = m_LineCount > m_PageInfo.lines.used - 1 ? m_PageInfo.lines.used - 1 : m_LineCount;
            if (m_PageInfo.lines.used - cnt > 0)
                RemoveLines(0, m_PageInfo.lines.used - cnt);
            ASSERT(m_PageInfo.lines.used > 0);

            for (i = 0; i < m_PageInfo.lines.used; i++)
            {
                h -= m_PageInfo.lines.lines[i].cy;
                h -= m_PageInfo.lines.lines[i].gap;
            }

            p_line = &m_PageInfo.lines.lines[m_PageInfo.lines.used-1];
            start_pos = p_line->start + p_line->length;

        }
        else
        {
            ClearLines();
            start_pos = m_Index + m_PageLength;
        }
    }
    else if (m_DrawType == DRAW_LINE_DOWN)
    {
        cnt = m_LineCount > m_PageInfo.lines.used ? m_PageInfo.lines.used : m_LineCount;
        if (cnt < m_PageInfo.lines.used)
        {
            RemoveLines(0, cnt);

            ASSERT(m_PageInfo.lines.used > 0);

            for (i = 0; i < m_PageInfo.lines.used; i++)
            {
                h -= m_PageInfo.lines.lines[i].cy;
                h -= m_PageInfo.lines.lines[i].gap;
            }

            p_line = &m_PageInfo.lines.lines[m_PageInfo.lines.used-1];
            start_pos = p_line->start + p_line->length;
        }
        else
        {
            ClearLines();
            start_pos = m_Index + m_PageLength;
        }
    }
    else
    {
        return;
    }

    if (CHAPTER_PAGE)
    {
        if (GetChapterInfo(0, &m_ChapterStart, &m_ChapterLength))
        {
            if (m_ChapterStart + m_ChapterLength == m_Index + m_PageLength && m_PageInfo.lines.used == 0 && m_DrawType != DRAW_NULL)
            {
                if (GetChapterInfo(1, &m_ChapterStart, &m_ChapterLength))
                {
                }
                else
                {
                    m_ChapterStart = GetTextBeginIndex();
                    m_ChapterLength = m_Length - GetTextBeginIndex();
                }
            }
        }
        else
        {
            m_ChapterStart = GetTextBeginIndex();
            m_ChapterLength = m_Length - GetTextBeginIndex();
        }
    }
    
    line_idx = m_PageInfo.lines.used;
    if (m_PageInfo.lines.used == 0)
        m_PageInfo.start = start_pos;
    else
        m_PageInfo.start = -1;

    while ((length = GetNextParagraph(start_pos, max_page_length, &is_blank_line, &crlf_len)) > 0) // out flag: at end of text
    {
        if (is_blank_line)
        {
            if (NO_BLANS_LINE)
            {
                remain_blank_length += length;
                start_pos += length;
                continue;
            }
            else
            {
                remain_blank_length = 0;
                // add blank line
                SelectFont(hdc, start_pos, FALSE);
                GetTextExtentPoint32(hdc, m_Text + start_pos, 1, &sz);
                if (h >= sz.cy)
                {
                    AddCharsToLine(line_idx++, NULL, 0, 0, start_pos, length, 0, sz.cy, 0);
                    h -= sz.cy;
                    if (h == sz.cy)
                    {
                        break; // completed
                    }
                }
                else
                {
                    break; // completed
                }
            }
        }
        else
        {
            remain_blank_length = 0;
            ASSERT(length - crlf_len > 0);
            line_cnt = ParagraphToLines(hdc, start_pos, length - crlf_len, width, h, line_idx);
            if (line_cnt == 0) // An error occurred(width is not enough) or completed(height is not enough), Otherwise, there is at least one line
            {
                break;
            }
            if (line_cnt > 0)
            {
                p_line = &m_PageInfo.lines.lines[m_PageInfo.lines.used - 1];
                if (p_line->start + p_line->length == start_pos + length - crlf_len) // all paragraph already in lines
                {
                    ASSERT(line_idx + line_cnt - 1 == m_PageInfo.lines.used - 1);
                    if (crlf_len)
                    {
                        p_line->gap = PART_GAP;
                        p_line->length += crlf_len; // add CRLF to lastest line
                    }
                }
                else
                {
                    break; // completed
                }
            }
        }

        // set new height
        h = height;
        for (i = 0; i < m_PageInfo.lines.used; i++)
        {
            h -= m_PageInfo.lines.lines[i].cy;
            h -= m_PageInfo.lines.lines[i].gap;
        }

        start_pos += length;
        line_idx = m_PageInfo.lines.used;
    }

    if (m_PageInfo.lines.used > 0)
    {
        p_line = &m_PageInfo.lines.lines[m_PageInfo.lines.used - 1];
        if (m_PageInfo.start == -1 || m_PageInfo.start > m_PageInfo.lines.lines[0].start)
            m_PageInfo.start = m_PageInfo.lines.lines[0].start;
        m_PageInfo.length = p_line->start + p_line->length - m_PageInfo.start + remain_blank_length;
    }
    else
    {
#if 0
        m_PageInfo.start = m_Index;
        m_PageInfo.length = m_PageLength;
#else
        m_PageInfo.start = m_Index + m_PageLength;
        m_PageInfo.length = 0;
#endif
        return;
    }
}

void Page::CalcPageUp(HDC hdc, RECT *rc)
{
    int width = rc->right - rc->left - LEFT_MIN - RIGHT_MIN;
    int height = rc->bottom - rc->top - TOP_MIN - BOTTOM_MIN;
    int start_pos, length;
    int is_blank_line = 1;
    int check_remain_blank = 1;
    int remain_blank_length = 0;
    int check_not_enough = 1;
    int crlf_len = 0;
    int line_idx = 0;
    int max_page_length = GetMaxPageLength(hdc, width, height);
    int i, line_cnt, cnt, h, idx = -1;
    SIZE sz;
    line_info_t *p_line;

    if (m_Index <= GetTextBeginIndex())
        return;

    if (m_DrawType == DRAW_PAGE_UP)
    {
        // for DRAW_PAGE_UP m_LineCount use for 'left lines count'
        if (m_LineCount > 0 && m_PageInfo.lines.used > 0 && !CHAPTER_PAGE)
        {
            cnt = m_LineCount > m_PageInfo.lines.used - 1 ? m_PageInfo.lines.used - 1 : m_LineCount;
            if (m_PageInfo.lines.used - cnt > 0)
                RemoveLines(cnt, m_PageInfo.lines.used - cnt); // remove from index = cnt to end().
            ASSERT(m_PageInfo.lines.used > 0);
        }
        else
        {
            ClearLines();
        }
    }
    else if (m_DrawType == DRAW_LINE_UP)
    {
        cnt = 0 - m_LineCount;
    }
    else
    {
        return;
    }

    if (CHAPTER_PAGE)
    {
        if (GetChapterInfo(0, &m_ChapterStart, &m_ChapterLength))
        {
            if (m_ChapterStart == m_Index)
            {
                if (GetChapterInfo(-1, &m_ChapterStart, &m_ChapterLength))
                {
                    ClearLines();
                }
                else
                {
                    m_ChapterStart = GetTextBeginIndex();
                    m_ChapterLength = m_Length - GetTextBeginIndex();
                }
            }
        }
        else
        {
            m_ChapterStart = GetTextBeginIndex();
            m_ChapterLength = m_Length - GetTextBeginIndex();
        }
    }

    start_pos = m_Index - 1;
    line_idx = 0;
    m_PageInfo.start = -1;

    while ((length = GetPrevParagraph(start_pos, max_page_length, &is_blank_line, &crlf_len)) > 0) // out flag: at begin of text
    {
        if (is_blank_line)
        {
            if (NO_BLANS_LINE)
            {
                m_PageInfo.start = start_pos - length + 1;
                start_pos -= length;
                if (m_PageInfo.lines.used == 0)
                    remain_blank_length += length;
                continue;
            }
            else
            {
                // add blank line
                SelectFont(hdc, start_pos, FALSE);
                GetTextExtentPoint32(hdc, m_Text + (start_pos - length + 1), 1, &sz);
                AddCharsToLine(line_idx, NULL, 0, 0, start_pos - length + 1, length, 0, sz.cy, 0);
                line_cnt = 1;
            }
        }
        else
        {
            ASSERT(length - crlf_len > 0);
            line_cnt = ParagraphToLines(hdc, start_pos - length + 1, length - crlf_len, width, INT_MAX, line_idx);
            if (line_cnt == 0) // An error occurred(width is not enough), Otherwise, there is at least one line
            {
                // donot check
                check_remain_blank = 0;
                check_not_enough = 0;
                ClearLines();
                break;
            }
            if (line_cnt > 0)
            {
                ASSERT(line_idx + line_cnt - 1 == line_cnt - 1);
                p_line = &m_PageInfo.lines.lines[line_idx + line_cnt - 1];
                if (crlf_len > 0)
                {
                    p_line->gap = PART_GAP;
                    p_line->length += crlf_len; // add CRLF to lastest line
                }
            }
        }

        if (m_DrawType == DRAW_LINE_UP)
        {
            cnt += line_cnt;

            if (cnt >= 0)
            {
                if (cnt > 0)
                {
                    ASSERT(m_PageInfo.lines.used > cnt);
                    p_line = &m_PageInfo.lines.lines[line_idx + line_cnt - 1];
                    RemoveLines(0, cnt);
                    cnt = 0;
                    ASSERT(p_line->start + p_line->length - (start_pos - length + 1) > 0);
                    check_remain_blank = 0;
                }
                ASSERT(m_PageInfo.lines.used > 0);

                // check if line are out of height
                idx = -1;
                h = 0;
                for (i = 0; i < m_PageInfo.lines.used; i++)
                {
                    h += m_PageInfo.lines.lines[i].cy;
                    if (h > height)
                    {
                        idx = i;
                        break;
                    }
                    h += m_PageInfo.lines.lines[i].gap;
                }
                if (idx != -1)
                {
                    RemoveLines(idx, m_PageInfo.lines.used - idx); // remove from index = idx to end().
                    check_not_enough = 0;
                }
                break;
            }
        }
        else // DRAW_PAGE_UP
        {
            // check height
            h = 0;
            idx = -1;
            for (i = m_PageInfo.lines.used - 1; i >= 0; i--)
            {
                h += m_PageInfo.lines.lines[i].cy;
                if (h > height)
                {
                    idx = i + 1;
                    break;
                }
                if (i > 0)
                    h += m_PageInfo.lines.lines[i - 1].gap;
            }
            if (idx != -1)
            {
                // completed
                RemoveLines(0, idx);
                check_not_enough = 0;
                break;
            }
        }

        start_pos -= length;
    }

    if (m_PageInfo.lines.used > 0)
    {
        p_line = &m_PageInfo.lines.lines[m_PageInfo.lines.used - 1];
        if (m_PageInfo.start == -1 || m_PageInfo.start > m_PageInfo.lines.lines[0].start)
            m_PageInfo.start = m_PageInfo.lines.lines[0].start;
        m_PageInfo.length = p_line->start + p_line->length - m_PageInfo.start + remain_blank_length;
    }
    else
    {
#if 0
        m_PageInfo.start = m_Index;
        m_PageInfo.length = m_PageLength;
#else
        m_PageInfo.start = m_Index;
        m_PageInfo.length = 0;
#endif
        return;
    }

    // add remaining blank lines for NO_BLANS_LINE 
    if (NO_BLANS_LINE && check_remain_blank && m_PageInfo.lines.used > 0)
    {
        start_pos = m_PageInfo.start - 1;
        is_blank_line = 1;
        while (start_pos >= GetTextBeginIndex() && is_blank_line && (length = GetPrevParagraph(start_pos, max_page_length, &is_blank_line, &crlf_len)) > 0)
        {
            if (is_blank_line)
            {
                m_PageInfo.length += length;
                m_PageInfo.start -= length;
                start_pos -= length;
            }
        }
    }

    // check if not enough
    if (check_not_enough && m_PageInfo.lines.used > 0)
    {
        // reduce logical complexity ... using DRAW_NULL
        m_Index = m_PageInfo.start;
        m_DrawType = DRAW_NULL;
        m_LineCount = 0;
        CalcPageDown(hdc, rc);
    }
}

int Page::GetMaxPageLength(HDC hdc, int w, int h)
{
    SIZE sz = { 0 };
    INT wcnt;
    INT hcnt;

    SelectFontByDcIndex(hdc, 0);
    GetTextExtentPoint32(hdc, _T("i"), 1, &sz);

    if (sz.cx == 0)
        sz.cx = 1;
    wcnt = w / (sz.cx + CHAR_GAP);
    hcnt = h / (sz.cy + LINE_GAP);
    return ((wcnt * hcnt) + 1023) / 1024 * 1024;
}

int Page::GetIndentWidth(HDC hdc)
{
    SIZE sz = { 0 };
    TCHAR buf[3] = { 0x3000, 0x3000, 0 };

    SelectFontByDcIndex(hdc, 0);
    GetTextExtentPoint32(hdc, buf, 2, &sz);
    return sz.cx;
}

#if ENABLE_TAG
int Page::IsTag(int index)
{
    int i, j;
    int len, begin, end;
    for (i = 0; i < TAG_COUNT; i++)
    {
        if (TAGS[i].enable && TAGS[i].keyword[0])
        {
            len = wcslen(TAGS[i].keyword);
            begin = index - len + 1 > 0 ? index - len + 1 : 0;
            end = index + 1/*+ len - 1 > m_TextLength ? m_TextLength : index + len - 1*/;

            for (j = begin; j < end; j++)
            {
                if (wcsncmp(TAGS[i].keyword, m_Text + j, len) == 0)
                    return i;
            }
        }
    }
    return -1;
}
#endif

BOOL Page::IsCover(int index)
{
    return index == 0 && GetCover();
}

BOOL Page::IsTitle(int index)
{
    return IsChapterIndex(index);
}

BOOL Page::IsNewLine(wchar_t c)
{
    return c == 0x0A || c == 0x0D;
}

BOOL Page::IsBlankLine(int start, int length)
{
    int i;
    if (start >= 0 && length > 0 && m_Text && start + length <= m_Length)
    {
        for (i = 0; i < length; i++)
        {
            if (!is_blank(m_Text[start + i]) && !IsNewLine(m_Text[start + i]))
                return FALSE;
        }
    }
    return TRUE;
}

void Page::TestCase(RECT* rc)
{
#if TEST_MODEL
    int w, h, i, j, l, v1, v2, v3, h1, w1;
    line_info_t *p_line;
    char_info_t *p_char;
    w = rc->right - rc->left - LEFT_MIN - RIGHT_MIN;
    h = rc->bottom - rc->top - TOP_MIN - BOTTOM_MIN;

    if (m_PageInfo.lines.used <= 0)
        return;

    h1 = 0;
    l = 0;
    for (i = 0; i < m_PageInfo.lines.used; i++)
    {
        p_line = &m_PageInfo.lines.lines[i];
        v1 = p_line->start;
        v2 = p_line->length;
        ASSERT(v1 >= 0 && v1 < m_Length);
        ASSERT(v2 >= p_line->char_cnt && v2 > 0 && p_line->char_cnt >= 0);
        l += p_line->length;
        h1 += p_line->cy;

        if (IsBlankLine(p_line->start, p_line->length))
        {
            ASSERT(p_line->gap == 0);
        }
        else if (m_Text[p_line->start + p_line->length - 1] == L'\n')
        {
            ASSERT(p_line->gap == PART_GAP);
        }
        else
        {
            ASSERT(p_line->gap == LINE_GAP);
        }

        if (i < m_PageInfo.lines.used - 1)
        {
            v3 = m_PageInfo.lines.lines[i + 1].start;
            if (NO_BLANS_LINE)
            {
                ASSERT(v1 + v2 <= v3);
                if (v1 + v2 < v3)
                {
                    ASSERT(IsBlankLine(v1 + v2, v3 - (v1 + v2)));
                }
            }
            else
            {
                ASSERT(v1 + v2 == v3);
            }
            h1 += p_line->gap;
        }

        w1 = p_line->x;
        for (j = 0; j < p_line->char_cnt; j++)
        {
            p_char = &p_line->chars[j];
            ASSERT(p_char->cy <= p_line->cy);
            ASSERT(p_char->idx >= GetTextBeginIndex() && p_char->idx < m_Length);
#if ENABLE_TAG
            ASSERT(p_char->dc_idx >= 0 && p_char->dc_idx < TAG_COUNT+2);
#else
            ASSERT(p_char->dc_idx >= 0 && p_char->dc_idx < 2);
#endif
            ASSERT(m_Text[p_char->idx] != L'\r' && m_Text[p_char->idx] != L'\n');
            w1 += p_char->cx;
            if (j < p_line->char_cnt - 1)
            {
                w1 += CHAR_GAP;
            }
        }
        ASSERT(w1 <= w);
    }
    ASSERT(h1 <= h);
    if (NO_BLANS_LINE)
        ASSERT(l <= m_PageInfo.length);
    else
        ASSERT(l == m_PageInfo.length);
#endif
}

BOOL Page::IsValid(void)
{
    return m_header && m_pIndex && m_Text && m_Length > 0;
}

BOOL Page::OnDrawPageEvent(HWND hWnd)
{
    return TRUE;
}

BOOL Page::OnUpDownEvent(HWND hWnd, int draw_type)
{
    return TRUE;
}

int Page::GetTextBeginIndex(void)
{
    return 0;
}

Gdiplus::Bitmap* Page::GetCover(void)
{
    return NULL;
}