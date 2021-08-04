#ifndef __ONLINE_BOOK_H__
#define __ONLINE_BOOK_H__
#ifdef ENABLE_NETWORK

#include "Book.h"
#include "httpclient.h"
#include "HtmlParser.h"
#include <set>

typedef enum comp_todo_t
{
    todo_nothing,
    todo_jump,
    todo_pageup,
    todo_pagedown,
    todo_lineup,
    todo_linedown
} comp_todo_t;

typedef void (*olbook_checkupdate_callback)(int is_update, int err, void *param);

class OnlineBook : public Book
{
public:
    OnlineBook();
    virtual ~OnlineBook();

public:
    virtual book_type_t GetBookType(void);
    virtual bool SaveBook(HWND hWnd);
    virtual bool UpdateChapters(int offset);
    virtual bool IsLoading(void);
    virtual void JumpChapter(HWND hWnd, int index);
    virtual void JumpPrevChapter(HWND hWnd);
    virtual void JumpNextChapter(HWND hWnd);
    virtual int GetCurChapterIndex(void);
    virtual LRESULT OnBookEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
    virtual bool ParserBook(HWND hWnd);
    bool ParserChapterPage(HWND hWnd, int idx); // chapter index
    bool ParserChapters(HWND hWnd, int idx); // chapter index
    bool ParserContent(HWND hWnd, int idx, u32 todo = todo_nothing); // chapter index
    bool ParserBookStatus(HWND hWnd);
    bool ReadOlFile(BOOL fast=FALSE);
    bool WriteOlFile();
    bool GenerateOlHeader(ol_header_t **header);
    bool ParseOlHeader(ol_header_t *header);
    bool DownloadPrevNext(HWND hWnd);
    virtual bool OnDrawPageEvent(HWND hWnd);
    virtual bool OnLineUpDownEvent(HWND hWnd, BOOL up, int n);
    void FormatHtml(char **html, int *len, int *needfree);
    void TidyHtml(char* html, int* len);
    void TidyUrl(char* html, int* len);
    void PlayLoading(HWND hWnd);
    void StopLoading(HWND hWnd, int idx);
    BOOL Redirect(OnlineBook* _this, request_t *r, const char *url, req_handler_t hOld);

public:
    void UpdateBookSource(void);
    int CheckUpdate(HWND hWnd, olbook_checkupdate_callback cb, void* arg);

private:
    static unsigned int GetChapterPageCompleter(request_result_t *result);
    static unsigned int GetChaptersCompleter(request_result_t *result);
    static unsigned int GetContentCompleter(request_result_t *result);
    static unsigned int GetBookStatusCompleter(request_result_t *result);

protected:
    HANDLE m_hEvent;
    HANDLE m_hMutex;
    std::set<req_handler_t> m_hRequestList;
    bool m_result;
    char m_MainPage[1024];
    char m_ChapterPage[1024];
    TCHAR m_BookName[256];
    char m_Host[1024];
    u64 m_UpdateTime;
    u32 m_IsFinished;
    BOOL m_IsLoading;
    int m_TagetIndex;
    book_source_t* m_Booksrc;
    olbook_checkupdate_callback m_cb;
    void* m_arg;
    BOOL m_IsCheck;
};

#endif
#endif
