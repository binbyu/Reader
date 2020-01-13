#ifndef __BOOK_H__
#define __BOOK_H__

#include <map>
#include "types.h"
#include "PageCache.h"
#include <string>


typedef struct chapter_item_t
{
    int index;
    std::wstring title;
} chapter_item_t;
typedef std::map<int, chapter_item_t> chapters_t;

typedef enum book_type_t
{
    book_unknown,
    book_text,
    book_epub
} book_type_t;

class Book : public PageCache
{
public:
    Book();
    virtual ~Book();

public:
    virtual book_type_t GetBookType(void) = 0;
    bool OpenBook(HWND hWnd);
    bool OpenBook(char *data, int size, HWND hWnd);
    bool CloseBook(void);
    bool IsLoading(void);
    void SetMd5(u128_t *md5);
    u128_t * GetMd5(void);
    void SetFileName(const TCHAR *fileName);
    TCHAR * GetFileName(void);
    wchar_t * GetText(void);
    int GetTextLength(void);
    chapters_t * GetChapters(void);
    void JumpChapter(HWND hWnd, int index);
    void JumpPrevChapter(HWND hWnd);
    void JumpNextChapter(HWND hWnd);
    int GetCurChapterIndex(void);
    static bool CalcMd5(TCHAR *fileName, u128_t *md5, char **data, int *size);

protected:
    virtual bool ParserBook(void) = 0;
    // srcsize and dstsize not include \0
    virtual bool DecodeText(const char *src, int srcsize, wchar_t **dst, int *dstsize);
    virtual bool FormatText(wchar_t *text, int *len);
    bool IsBlanks(wchar_t c);
    void ForceKill(void);

protected:
    static unsigned __stdcall OpenBookThread(void* pArguments);

protected:
    wchar_t m_fileName[MAX_PATH];
    chapters_t m_Chapters;
    char *m_Data;
    int m_Size;
    HANDLE m_hThread;
    u128_t m_md5;
    bool m_bForceKill;
};

typedef struct ob_thread_param_t
{
    Book *_this;
    HWND hWnd;
} ob_thread_param_t;

#endif
