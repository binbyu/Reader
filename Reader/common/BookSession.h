#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include "framework.h"
#include "IBook.h"
#include "ILayout.h"
#include "IRenderer.h"

enum class BookState
{
    CLOSED,
    LOADING,
    OPENED,
    UNREADY,
    FAILED
};

class BookSession
{
private:
    enum FetchType
    {
        SEEK_CHAPTER,
        PREV_PAGE,
        PREV_LINE,
        NEXT_PAGE,
        NEXT_LINE
    };
public:
    BookSession(HWND hWnd);
    ~BookSession();

public:
    void              init();
    void              uninit();
    void              open(int index);
    void              open(const std::wstring& fileName, bool forced = false);
    void              reopen();
    void              close();
    void              remove(const std::wstring& fileName);
    void              remove(int index);
    void              clear();
    void              refreshChapter();
    void              refreshContent();
    void              checkBookUpdate();

    BookState         state() const;
    BookErrno         lastError() const;
    bool              isLoading() const;
    bool              isValid() const;
    bool              isEnd() const;

    IBook*            book() const;
    float             progress() const;
    std::wstring      progressStr() const;

    void              seek(int index);
    void              seek(float prog);
    void              seek(bool forward, const wchar_t* keyword);
    void              seekChapter(int index);
    void              nextLine(int n = 1);
    void              prevLine(int n = 1);
    void              nextPage();
    void              prevPage();
    void              nextChapter();
    void              prevChapter();

    const Marks&      marks();
    void              addMark(int index);
    void              remMark(int index);
    void              clrMark();

    int               pageLength() const;
    const PageInfo&   pageInfo() const;
    void              drawPage(HDC hdc, const RECT& rect);

    void              updateStyles();
    const TextStyles& styles() const;

    LRESULT           onEvent(WPARAM wParam, LPARAM lParam);
    LRESULT           onTimer(WPARAM wParam, LPARAM lParam);

private:
    void              addBookItem(const std::wstring& fileName);
    void              remBookItem(const std::wstring& fileName);
    void              clrBookItem();
    void              setState(BookState state);
    void              setError(BookErrno err);
    void              setTextStyles();
    bool              isChapterReady(int chapterIdx);
    bool              fetchChapter(FetchType type, int chapterIdx = -1, int n = 1);
    void              preFetchChapter(int chapterIdx);
    void              checkNext();

private:
    HWND              m_hWnd;
    ILayout*          m_layout;
    IBook*            m_book;
    BookInfo*         m_bookInfo;
    BookState         m_state;
    BookErrno         m_errno;
    PageInfo          m_pageInfo;
    TextStyles        m_styles;
    int               m_width;
    int               m_height;
    std::thread       m_thread;
    std::mutex        m_mutex;
    std::vector<std::wstring> m_checkUpdateQueue;
};