#ifndef __PAGE_H__
#define __PAGE_H__

#include <vector>
#include "types.h"

typedef struct char_info_t
{
    int idx;
    int dc_idx;
    int cx;
    int cy;
} char_info_t;

typedef struct line_info_t
{
    int start;
    int length;
    int x;
    int cy;
    int gap;
    char_info_t *chars;
    int char_cnt;
} line_info_t;

typedef struct lines_t
{
    line_info_t *lines;
    int total;
    int used;
} lines_t;

typedef struct page_info_t
{
    int start;
    int length;
    lines_t lines;
} page_info_t;

typedef struct dc_info_t
{
    HFONT hFont;
    DWORD BkColor;
    DWORD TextColor;
    DWORD BkMode;
} dc_info_t;

typedef struct alpha_dc_info_t
{
    HBITMAP hDIB;
    BYTE *pvBits;
    int width;
    int height;
} alpha_dc_info_t;

#define DRAW_NULL               0
#define DRAW_PAGE_DOWN          1
#define DRAW_PAGE_UP            2
#define DRAW_LINE_DOWN          3
#define DRAW_LINE_UP            4

#define m_Index                 (*m_pIndex)

#define is_space(c)             ((c) == 0x20 || (c) == 0x09 /*|| (c) == 0x0A*/ || (c) == 0x0B || (c) == 0x0C /*|| (c) == 0x0D*/)
#define is_hyphen(c)            ((c) == 0x2D /* - */)
#define is_blank(c)             ((c) == 0x20 || (c) == 0x09 || /*(c) == 0x0A ||*/ (c) == 0x0B || (c) == 0x0C || /*(c) == 0x0D ||*/ (c) == 0x3000 || (c) == 0xA0)

class Page
{
public:
    Page();
    virtual ~Page();

    void Init(int *p_index, header_t *header);
    void PageUp(HWND hWnd, BOOL draw = TRUE);
    void PageDown(HWND hWnd, BOOL draw = TRUE);
    void LineUp(HWND hWnd, BOOL draw = TRUE);
    void LineDown(HWND hWnd, BOOL draw = TRUE);
    void DrawPage(HWND hWnd, HDC hdc, RECT *rc, BOOL enable_alpha);
    void ReDraw(HWND hWnd);
    int  GetPageLength(void);
    int  GetTextLength(void);
    BOOL IsFirstPage(void);
    BOOL IsLastPage(void);
    BOOL IsCoverPage(void);
    double GetProgress(void);
    BOOL GetCurPageText(TCHAR **text);
    BOOL SetCurPageText(HWND hWnd, TCHAR *text);
    BOOL IsBlankPage(void);

protected:
    BOOL DrawCover(HDC hdc, RECT *rc);
    void CreateAlphaTextBitmap(HDC hdc, int width, int height, alpha_dc_info_t *p_alpha_dc);
    void DeleteAlphaTextBitmap(HDC hdc, alpha_dc_info_t *p_alpha_dc);
    void DrawAlphaText(HDC hdc, char_info_t* p_char, int x, int y, int h, alpha_dc_info_t *p_alpha_dc);
    void BeginDraw(void);
    void EndDraw(void);
    DWORD GetTextAlpha(DWORD color);
    int  SelectFont(HDC hdc, int index, BOOL is_title);
    void SelectFontByDcIndex(HDC hdc, int dc_idx);
    int  GetPrevParagraph(int start, int max_len, int *is_blank, int *crlf_len);
    int  GetNextParagraph(int start, int max_len, int *is_blank, int *crlf_len);
    int  ParagraphToLines(HDC hdc, int start, int end, int width, int height, int line_idx);
    int  AddCharsToLine(int line_idx, char_info_t *chars, int char_start, int char_len, int line_start, int line_len, int x, int cy, int gap);
    void RemoveLines(int line_idx, int count);
    void ClearLines(void);
    void ReleasePageInfo(void);
    void CalcPageDown(HDC hdc, RECT *rc);
    void CalcPageUp(HDC hdc, RECT *rc);
    int  GetMaxPageLength(HDC hdc, int w, int h);
    int  GetIndentWidth(HDC hdc);

#if ENABLE_TAG
    int  IsTag(int index);
#endif
    BOOL IsCover(int index);
    BOOL IsTitle(int index);
    BOOL IsNewLine(wchar_t c);
    BOOL IsBlankLine(int start, int length);

    void TestCase(RECT *rc);

protected:
    virtual BOOL IsValid(void);
    virtual BOOL OnDrawPageEvent(HWND hWnd);
    virtual BOOL OnUpDownEvent(HWND hWnd, int draw_type);
    virtual Gdiplus::Bitmap* GetCover(void);
    virtual int  GetTextBeginIndex(void);
    virtual BOOL IsChapterIndex(int index) = 0;
    virtual BOOL IsChapter(int index) = 0;
    virtual BOOL GetChapterInfo(int type, int *start, int *length) = 0; // type=0 curn chapter, type=1 next chapter, type=-1, prev chapter

protected:
    wchar_t* m_Text;
    int m_Length;
    int *m_pIndex;
    int m_PageLength;
    header_t *m_header;

private:
    page_info_t m_PageInfo;
    dc_info_t *m_dcList;
    int m_dcIndex;
    int m_LineCount;
    int m_DrawType;
    BOOL m_BlankPage;
    int m_ChapterStart;     // for CHAPTER_PAGE
    int m_ChapterLength;    // CHAPTER_PAGE
};

#endif
