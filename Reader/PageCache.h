#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

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
    ~PageCache();

public:
    void SetText(HWND hWnd, TCHAR *text, INT size, INT *pos, INT *lg, INT *ib);
    void SetRect(RECT *rect);
    void Reset(HWND hWnd, BOOL redraw = TRUE);
    void ReDraw(HWND hWnd);
    void PageUp(HWND hWnd);
    void PageDown(HWND hWnd);
    void LineUp(HWND hWnd, INT n);
    void LineDown(HWND hWnd, INT n);
    void DrawPage(HDC hdc);
    INT GetCurPageSize(void);
    INT GetOnePageLineCount(void);

private:
    LONG GetLineHeight(HDC hdc);
    INT GetCahceUnitSize(HDC hdc);
    void LoadPageInfo(HDC hdc, INT maxw);
    void AddLine(INT start, INT length, INT pos = -1);
    void RemoveAllLine(BOOL freemem = FALSE);
    BOOL IsValid(void);

private:
    void UnitTest1(void);
    void UnitTest2(void);

private:
    TCHAR *m_Text;
    INT m_Size;
    RECT m_Rect;
    INT m_OnePageLineCount;
    INT m_CurPageSize;
    INT m_CurrentLine;
    INT *m_CurrentPos;
    INT *m_lineGap;
    INT *m_InternalBorder;
    page_info_t m_PageInfo;
};

#endif
