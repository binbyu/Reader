#ifndef __BOOK_H__
#define __BOOK_H__

#include <map>
#include "types.h"
#include "Page.h"
#include <string>


typedef struct chapter_item_t
{
    int index;
    std::wstring title;
    std::string url; // for online book
    int size; // current chapter total len, for online book
    int title_len;
} chapter_item_t;
typedef std::vector<chapter_item_t> chapters_t;

typedef enum book_type_t
{
    book_unknown,
    book_text,
    book_epub,
    book_mobi,
    book_online
} book_type_t;

class Book;
struct book_event_data_t
{
    Book* _this;
    int ret;
    int is_updated;

    book_event_data_t()
    {
        _this = NULL;
        ret = 0;
        is_updated = 0;
    }
};

typedef struct file_data_t
{
    void *data;
    int size;
} file_data_t;
typedef std::map<std::string, file_data_t> filelist_t;

typedef struct manifest_t
{
    std::string id;
    std::string href;
    std::string media_type;
} manifest_t;
typedef std::map<std::string, manifest_t *> manifests_t;

typedef std::vector<std::string> spines_t;

typedef struct navpoint_t
{
    std::string id;
    std::string src;
    std::string text;
    int order;
} navpoint_t;
typedef std::map<std::string, navpoint_t *> navpoints_t;


class Book : public Page
{
public:
    Book();
    virtual ~Book();

public:
    virtual book_type_t GetBookType(void) = 0;
    virtual BOOL SaveBook(HWND hWnd) = 0;
    virtual BOOL UpdateChapters(int offset) = 0;
    BOOL OpenBook(HWND hWnd);
    BOOL OpenBook(char *data, int size, HWND hWnd);
    BOOL CloseBook(void);
    virtual BOOL IsLoading(void);
    void SetFileName(const TCHAR *fileName);
    TCHAR * GetFileName(void);
    wchar_t * GetText(void);
    int GetTextLength(void);
    chapters_t * GetChapters(void);
    void SetChapterRule(chapter_rule_t *rule);
    virtual void JumpChapter(HWND hWnd, int index);
    virtual void JumpPrevChapter(HWND hWnd);
    virtual void JumpNextChapter(HWND hWnd);
    virtual int GetCurChapterIndex(void);
    virtual LRESULT OnBookEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    BOOL GetChapterTitle(TCHAR *title, int size);
    BOOL FormatText(wchar_t *p_data, int *p_len);

protected:
    virtual BOOL ParserBook(HWND hWnd) = 0;
    // srcsize and dstsize not include \0
    virtual BOOL DecodeText(const char *src, int srcsize, wchar_t **dst, int *dstsize);
    virtual BOOL IsChapterIndex(int index);
    virtual BOOL IsChapter(int index);
    virtual BOOL GetChapterInfo(int type, int *start, int *length);
    virtual BOOL IsValid(void);
    
    BOOL GetLine(wchar_t* text, int len, int *line_len, int *lf_len, int *is_blank_line, int *prefix_blank_len, int *suffix_blank_len);
    void ForceKill(void);

protected:
    static unsigned __stdcall OpenBookThread(void* pArguments);

protected:
    wchar_t m_fileName[MAX_PATH];
    chapters_t m_Chapters;
    char *m_Data;
    int m_Size;
    HANDLE m_hThread;
    BOOL m_bForceKill;
    chapter_rule_t *m_Rule;
};

typedef struct ob_thread_param_t
{
    Book *_this;
    HWND hWnd;
} ob_thread_param_t;

#endif
