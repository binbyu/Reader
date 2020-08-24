#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

#include "types.h"

#define TEST_MODEL      0
#define FAST_MODEL      1

typedef struct line_info_t
{
    INT start;
    INT length;
} line_info_t;

typedef struct page_info_t
{
    INT line_size;
    INT alloc_size;
    line_info_t *line_info;
} page_info_t;

typedef enum page_status_t
{
    INIT = 0,
    DOWN = 1,
    UP = 2,
} page_status_t;


class PageCache
{
public:
    PageCache();
    virtual ~PageCache();

public:
#if ENABLE_TAG
    void Setting(HWND hWnd, INT *pos, INT *lg, INT *ib, tagitem_t *tags);
#else
    void Setting(HWND hWnd, INT *pos, INT *lg, INT *ib);
#endif
    void SetRect(RECT *rect);
    void SetLeftLine(int lines);
    void Reset(HWND hWnd, BOOL redraw = TRUE);
    void ReDraw(HWND hWnd);
    void PageUp(HWND hWnd);
    void PageDown(HWND hWnd);
    void LineUp(HWND hWnd, INT n);
    void LineDown(HWND hWnd, INT n);
    void DrawPage(HDC hdc);
    INT GetCurPageSize(void);
    INT GetOnePageLineCount(void);
    INT GetTextLength(void);
    BOOL IsFirstPage(void);
    BOOL IsLastPage(void);
    BOOL IsCoverPage(void);
    double GetProgress(void);
    BOOL GetCurPageText(TCHAR **text);
    BOOL SetCurPageText(HWND hWnd, TCHAR *text);

protected:
#if ENABLE_TAG
    LONG GetLineHeight(HDC hdc, HFONT *tagfonts);
#else
    LONG GetLineHeight(HDC hdc);
#endif
    INT GetCahceUnitSize(HDC hdc);
#if ENABLE_TAG
    void LoadPageInfo(HDC hdc, INT maxw, HFONT *tagfonts);
#else
    void LoadPageInfo(HDC hdc, INT maxw);
#endif
    void AddLine(INT start, INT length, INT pos = -1);
    void RemoveAllLine(BOOL freemem = FALSE);
    BOOL IsValid(void);
    Bitmap * GetCover(void);
    BOOL DrawCover(HDC hdc);
#if ENABLE_TAG
    int IsTag(int index);
#endif

protected:
    void UnitTest1(void);
    void UnitTest2(void);

protected:
    wchar_t * m_Text;
    INT m_TextLength;
    RECT m_Rect;
    INT m_OnePageLineCount;
    INT m_CurPageSize;
    INT m_CurrentLine;
    INT *m_CurrentPos;
    INT *m_lineGap;
    INT *m_InternalBorder;
    INT m_LeftLineCount;
    page_info_t m_PageInfo;
#if ENABLE_TAG
    tagitem_t *m_tags;
#endif
};

#endif
