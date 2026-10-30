#pragma once
#ifdef ENABLE_NETWORK
#include "BookBase.h"
#include "OLFile.h"
#include "Settings.h"

#include "Logger.h"
#include "framework.h"
#include "https.h"
#include <list>

enum EventType
{
    UPDATE_CHAPTER,
    UPDATE_CONTENT,
    UPDATE_PROGRESS,
};

struct EventDataChapter
{
    Chapters      chapters;
};

struct EventDataContent
{
    int           chapterIdx;
    wchar_t*      text;
    int           length;
};

struct EventDataProgress
{
    WebShelfItem* item;
};

enum ReqType
{
    CHAPTERS,
    CONTENT,
    SETPROGS,
    GETPROGS,
};

struct ReqData
{
    BookBase*     book;
    ReqType       type;
    AsyncCallback cb;
    int           chapterIdx;
    std::vector<std::string> chapterTitles;
    std::vector<std::string> chapterUrls;
    std::vector<std::string> contents;
};

class ReqList
{
public:
    ReqList() {}
    ~ReqList()
    {
        clear();
        wait();
    }
    void wait()
    {
        // wait https completer callback
        // will block the UI, which isn’t great for user experience, but it’s safer.
        std::unique_lock<std::mutex> lock(m_mutex);
        auto ret = m_cv.wait_for(lock, std::chrono::seconds(5), [this] { return m_list.empty(); });
        if (!ret) // timeout
        {
            lgdebug("ReqList destructor timed out waiting for network requests to complete!");
            ASSERT(0);
        }
    }
    bool empty()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_list.empty();
    }
    void add(const request_t& r)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_list.push_back(https_request(&r));
    }
    void remove(request_handle_t h)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_list.remove(h);
        if (m_list.empty()) m_cv.notify_all();
    }
    void remove_and_add(request_handle_t h, const request_t& r)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_list.remove(h);
        m_list.push_back(https_request(&r));
    }
    void clear()
    {
        std::list<request_handle_t> copyList;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            copyList = m_list;
        }
        // https_cancel will call completer, completer will call remove
        for (auto h : copyList) https_cancel(h);
    }
    ReqData* find(ReqType type, int chapterIdx = -1)
    {
        request_t* r;
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto h : m_list)
        {
            r = https_get_request_info(h);
            if (!r) continue;
            ReqData* rd = static_cast<ReqData*>(r->param1);
            if (type == CHAPTERS && rd && rd->type == type) return rd;
            if (type == SETPROGS && rd && rd->type == type) return rd;
            if (type == GETPROGS && rd && rd->type == type) return rd;
            if (rd && rd->type == type && rd->chapterIdx == chapterIdx) return rd;
        }
        return nullptr;
    }
private:
    std::list<request_handle_t> m_list;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

class BookOnline : public BookBase
{
public:
    BookOnline(const std::wstring& fileName);
    virtual           ~BookOnline();
    virtual BookType  type() const;
    virtual void      close();
    virtual bool      isFetchSupport() const;
    virtual bool      fetchChapter(int chapterIndex, AsyncCallback cb = nullptr);
    virtual bool      isRefreshSupport() const;
    virtual bool      refreshChapter(AsyncCallback cb);
    virtual bool      refreshContent(int chapterIndex, AsyncCallback cb);
    virtual LRESULT   onEvent(WPARAM wParam, LPARAM lParam);
    virtual LRESULT   onTimer(WPARAM wParam, LPARAM lParam);
    virtual void      setHWND(HWND hWnd);
    virtual void      checkUpdate(AsyncCallback cb);

protected:
    virtual BookErrno parseBook();
    virtual BookErrno loadFile();
    virtual BookErrno saveFile();
    virtual BookErrno parseFile(const char* buffer, size_t size);
    virtual bool      parseChapters(int chapterIdx, AsyncCallback cb = nullptr);
    virtual bool      parseContent(int chapterIdx, AsyncCallback cb = nullptr);
    bool              parseMainPage(int chapterIdx, AsyncCallback cb = nullptr);
    bool              buildContent(const std::vector<std::string>& contents, int chapterIdx, wchar_t*& text, int& length);
    bool              filterContent(wchar_t* text, int& length);
    bool              isCurnReadingBook();
    BookErrno         readFile();
    BookErrno         writeFile();

protected:
    HWND                    m_hWnd;
    IFile*                  m_file;
    ReqList                 m_reqs;
    std::mutex              m_mutex;
    std::condition_variable m_cv;

private:
    BookSource*             m_bs;
    std::string             m_mainPage;
    std::string             m_chapterPage;
    uint64_t                m_updateTime;
    bool                    m_isCheckUpdate;
    int                     m_fetchFailCount;
};
#endif