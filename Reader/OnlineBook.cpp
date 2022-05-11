#ifdef ENABLE_NETWORK
#include "OnlineBook.h"
#include "Utils.h"
#include "resource.h"
#include <time.h>
#include <regex>
#include <shellapi.h>

extern BOOL PlayLoadingImage(HWND);
extern BOOL StopLoadingImage(HWND);
extern int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType);
extern book_source_t* FindBookSource(const char* host);
extern void DumpParseErrorFile(const char *html, int htmllen);
extern void UpdateBookMark(HWND hWnd, int index, int size);

int parse_protocol_host(const char* url, char* host)
{
    const char* p = NULL;
    const char* t = NULL;

    p = url;
    // is ssl
    if (0 == strncmp(url, "http://", 7))
    {
        p += 7;
    }
    else if (0 == strncmp(url, "https://", 8))
    {
        p += 8;
    }

    // parse host
    t = strstr(p, "/");
    if (t)
    {
        strncpy(host, url, t - url);
    }
    else
    {
        strcpy(host, url);
    }
    return succ;
}

void combine_url(const char* path, const char* url, char* dsturl)
{
    char temp[1024] = { 0 };
    char host[1024] = { 0 };
    char* p;
    if (_strnicmp("http", path, 4) == 0)
    {
        strcpy(dsturl, path);
    }
    else if (_strnicmp("//www.", path, 6) == 0
        || _strnicmp("/www.", path, 5) == 0
        || _strnicmp("//m.", path, 4) == 0
        || _strnicmp("/m.", path, 3) == 0)
    {
        p = (char*)strstr(path, "www.");
        if (0 == strncmp(url, "http://", 7))
        {
            sprintf(dsturl, "http://%s", p);
        }
        else if (0 == strncmp(url, "https://", 8))
        {
            sprintf(dsturl, "https://%s", p);
        }
        else
        {
            sprintf(dsturl, "http://%s", p);
        }
    }
    else if (_strnicmp("//m.", path, 4) == 0
        || _strnicmp("/m.", path, 3) == 0)
    {
        p = (char*)strstr(path, "m.");
        if (0 == strncmp(url, "http://", 7))
        {
            sprintf(dsturl, "http://%s", p);
        }
        else if (0 == strncmp(url, "https://", 8))
        {
            sprintf(dsturl, "https://%s", p);
        }
        else
        {
            sprintf(dsturl, "http://%s", p);
        }
    }
    else
    {
        if (path[0] == '/')
        {
            parse_protocol_host(url, host);
            sprintf(dsturl, "%s%s", host, path);
        }
        else
        {
            strcpy(temp, url);
            p = strrchr(temp, '/');
            if (p)
            {
                *(p + 1) = 0;
                sprintf(dsturl, "%s%s", temp, path);
            }
            else
            {
                parse_protocol_host(url, host);
                sprintf(dsturl, "%s/%s", host, path);
            }
        }
    }
}

#define check_request_result(r)     \
     if ((r)->cancel)               \
        goto end;                   \
    if ((r)->errno_ != succ)        \
        goto end;                   \
    if ((r)->status_code != 200)    \
        goto end;                   \
    html = (r)->body;               \
    htmllen = (r)->bodylen;         \
    if (!is_utf8(html, htmllen)) {  \
        wchar_t* tempbuf = NULL;    \
        int templen = 0;            \
        char* utf8buf = NULL;       \
        int utf8len = 0;            \
        tempbuf = ansi_to_utf16(html, htmllen, &templen);    \
        utf8buf = utf16_to_utf8(tempbuf, templen, &utf8len); \
        free(tempbuf);              \
        if (needfree) {free(html);} \
        html = utf8buf;             \
        htmllen = utf8len;          \
        needfree = 1;               \
    }                               \
    if (_this->m_bForceKill)        \
        goto end;

typedef struct req_chapter_param_t
{
    HWND hWnd;
    int index;
    OnlineBook* _this;
    std::vector<std::string> *title_list;
    std::vector<std::string> *title_url;
} req_chapter_param_t;

typedef struct req_content_param_t
{
    HWND hWnd;
    int index;
    u32 todo;
    OnlineBook* _this;
    TCHAR *text;
    int textlen;
} req_content_param_t;

typedef struct req_bookstatus_param_t
{
    HWND hWnd;
    OnlineBook* _this;
    int is_update;
} req_check_param_t;

typedef enum book_event_t
{
    BE_UPATE_CHAPTER,
    BE_UPATE_CONTENT,
    BE_PLAY_LOADING,
    BE_STOP_LOADING,
    BE_SAVE_FILE
} book_event_t;

struct content_data_t : public book_event_data_t
{
    int index; // chapter index
    TCHAR* text;
    int len;
    u32 todo;

    content_data_t()
    {
        index = -1;
        text = NULL;
        len = 0;
        todo = todo_nothing;
    }
};

struct chapter_data_t : public book_event_data_t
{
    chapters_t chapters;

    chapter_data_t()
    {
        chapters.clear();
    }
};

struct loading_data_t : public book_event_data_t
{
    int idx;
    loading_data_t()
    {
        idx = -1;
    }
};


OnlineBook::OnlineBook()
    : m_hEvent(NULL)
    , m_hMutex(NULL)
    , m_result(FALSE)
    , m_IsLoading(FALSE)
    , m_TagetIndex(-1)
    , m_Booksrc(NULL)
    , m_cb(NULL)
    , m_arg(NULL)
    , m_IsNotCurnOpenedBook(TRUE)
{
    memset(m_MainPage, 0, sizeof(m_MainPage));
    memset(m_ChapterPage, 0, sizeof(m_ChapterPage));
    memset(m_BookName, 0, sizeof(m_BookName));
    memset(m_Host, 0, sizeof(m_Host));
    m_hMutex = CreateMutex(NULL, FALSE, NULL);
}

OnlineBook::~OnlineBook()
{
    std::set<req_handler_t>::iterator it;

    for (it = m_hRequestList.begin(); it != m_hRequestList.end(); it++)
    {
        hapi_cancel(*it);
    }

    if (m_hEvent)
    {
        CloseHandle(m_hEvent);
        m_hEvent = NULL;
    }

    ForceKill();

    m_hRequestList.clear();
    
    if (m_hMutex)
    {
        CloseHandle(m_hMutex);
        m_hMutex = NULL;
    }
    m_result = FALSE;
}

book_type_t OnlineBook::GetBookType()
{
    return book_online;
}

BOOL OnlineBook::SaveBook(HWND hWnd)
{
    return FALSE;
}

BOOL OnlineBook::UpdateChapters(int offset)
{
    return FALSE;
}

BOOL OnlineBook::IsLoading(void)
{
    return m_hThread != NULL || m_IsLoading;
}

void OnlineBook::JumpChapter(HWND hWnd, int index)
{
    if (index < 0 || index >= (int)m_Chapters.size())
        return;

    if (m_Chapters[index].index == -1)
    {
        m_TagetIndex = index;
        ParserContent(hWnd, index, todo_jump);
        PlayLoading(hWnd);
    }
    else
    {
        Book::JumpChapter(hWnd, index);
    }
}

void OnlineBook::JumpPrevChapter(HWND hWnd)
{
    int cur = GetCurChapterIndex();
    int prev = cur - 1;

    if (cur == -1)
        return;

    if (m_Chapters.size() == 0)
        return;

    if (prev >= 0 && prev < (int)m_Chapters.size())
    {
        if (m_Chapters[prev].index == -1)
        {
            m_TagetIndex = prev;
            ParserContent(hWnd, prev, todo_jump);
            PlayLoading(hWnd);
        }
        else
        {
            m_Index = m_Chapters[prev].index;
            ReDraw(hWnd);
        }
    }
}

void OnlineBook::JumpNextChapter(HWND hWnd)
{
    int cur = GetCurChapterIndex();
    int next = cur + 1;

    if (cur == -1)
        return;

    if (m_Chapters.size() == 0)
        return;

    if (next >= 0 && next < (int)m_Chapters.size())
    {
        if (m_Chapters[next].index == -1)
        {
            m_TagetIndex = next;
            ParserContent(hWnd, next, todo_jump);
            PlayLoading(hWnd);
        }
        else
        {
            m_Index = m_Chapters[next].index;
            ReDraw(hWnd);
        }
    }
}

int OnlineBook::GetCurChapterIndex(void)
{
    int index = -1;
    int i;

    if (!m_Text)
        return index;

    if (m_Chapters.size() <= 0)
        return index;

    if (!m_pIndex)
        return index;

    index = 0;
    for (i = 0; i < (int)m_Chapters.size(); i++)
    {
        if (m_Chapters[i].index != -1 && m_Chapters[i].index > m_Index)
        {
            break;
        }
        if (m_Chapters[i].index != -1)
            index = i;
    }
    return index;
}

LRESULT OnlineBook::OnBookEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    chapter_data_t* chapters = NULL;
    content_data_t* content = NULL;
    loading_data_t* loading = NULL;
    size_t i;
    int offset = -1;
    int ret = 0;

    switch (wParam)
    {
    case BE_UPATE_CHAPTER:
        chapters = (chapter_data_t*)lParam;
        if (!chapters)
            break;
        if (m_Chapters.empty())
        {
            m_Chapters = chapters->chapters;
        }
        else
        {
            // check update
            if (m_Chapters.size() >= chapters->chapters.size())
            {
                chapters->ret = 1;
                break; // invalid data
            }
            for (i = 0; i < m_Chapters.size(); i++)
            {
                ASSERT(m_Chapters[i].title == (chapters->chapters)[i].title);
                ASSERT(m_Chapters[i].url == (chapters->chapters)[i].url);

                // fixed
                if (m_Chapters[i].index == -1
                    && (m_Chapters[i].title != (chapters->chapters)[i].title || m_Chapters[i].url != (chapters->chapters)[i].url))
                {
                    m_Chapters[i].title = (chapters->chapters)[i].title;
                    m_Chapters[i].url = (chapters->chapters)[i].url;
                }

                /*  don't update menu
                if (m_Chapters[i].index == -1
                    && (m_Chapters[i].title != (chapters->chapters)[i].title))
                {
                    chapters->is_updated = 1; // have update
                }
                */
            }
            for (i = m_Chapters.size(); i < chapters->chapters.size(); i++)
            {
                chapters->is_updated = 1; // have update
                m_Chapters.push_back((chapters->chapters)[i]);
            }
        }
        WriteOlFile();
        break;
    case BE_UPATE_CONTENT:
        content = (content_data_t*)lParam;
        if (!content)
            break;
        if (content->index < 0 || content->index >= (int)m_Chapters.size())
            break;

        ASSERT(m_Chapters[content->index].index == -1);

        if (m_Text == NULL) // update text
        {
            m_Length = content->len;
            m_Text = (TCHAR*)malloc((m_Length + 1) * sizeof(TCHAR));
            memcpy(m_Text, content->text, (m_Length + 1) * sizeof(TCHAR));
            // update chapter index
            m_Chapters[content->index].index = 0;
            m_Chapters[content->index].size = content->len;
            ret = 1;
        }
        else // insert text
        {
            // fixed bug, Due to the delay of WM_PAINT processing.
            // if the BE_UPATE_CONTENT message is received before the Invalidate done refresh, the page operation will be lost
            UpdateWindow(hWnd);

            m_Length += content->len;
            m_Text = (TCHAR*)realloc(m_Text, (m_Length + 1) * sizeof(TCHAR));
            m_Text[m_Length] = 0;

            for (i = content->index + 1; i < m_Chapters.size(); i++)
            {
                if (m_Chapters[i].index != -1)
                {
                    if (offset == -1)
                    {
                        offset = m_Chapters[i].index;
                    }
                    m_Chapters[i].index += content->len;
                }
            }

            if (offset == -1) // append
            {
                m_Chapters[content->index].index = m_Length - content->len;
                m_Chapters[content->index].size = content->len;
                memcpy(m_Text + m_Chapters[content->index].index, content->text, sizeof(TCHAR) * content->len);
            }
            else // insert
            {
                m_Chapters[content->index].index = offset;
                m_Chapters[content->index].size = content->len;
                memcpy(m_Text + offset + content->len, m_Text + offset, sizeof(TCHAR) * (m_Length - offset - content->len));
                memcpy(m_Text + offset, content->text, sizeof(TCHAR) * content->len);

                // update book mark
                UpdateBookMark(hWnd, offset, content->len);
            }

            // update current pos
            if (m_pIndex)
            {
                if (m_Index >= m_Chapters[content->index].index)
                    m_Index += content->len;
            }
            ClearLines();
            // todo after request completed.
            switch (content->todo)
            {
            case todo_jump:
                if (m_pIndex)
                {
                    m_Index = m_Chapters[content->index].index; // will redraw by StopLoadingImage
                }
                break;
            case todo_pageup:
                PageUp(hWnd, FALSE); // will redraw by StopLoadingImage
                break;
            case todo_pagedown:
                PageDown(hWnd, FALSE); // will redraw by StopLoadingImage
                break;
            case todo_lineup:
                LineUp(hWnd, FALSE); // will redraw by StopLoadingImage
                break;
            case todo_linedown:
                LineDown(hWnd, FALSE); // will redraw by StopLoadingImage
                break;
            default:
                break;
            }

            if (m_pIndex)
                logger_printk("--- BE_UPATE_CONTENT: pos=%d, index=%d, size=%d, curpos=%d ---", content->index, m_Chapters[content->index].index, m_Chapters[content->index].size, m_Index);
            else
                logger_printk("--- BE_UPATE_CONTENT: pos=%d, index=%d, size=%d ---", content->index, m_Chapters[content->index].index, m_Chapters[content->index].size);
        }
        WriteOlFile();
        break;
    case BE_PLAY_LOADING:
        loading = (loading_data_t*)lParam;
        PlayLoadingImage(hWnd);
        if (loading)
            delete loading;
        break;
    case BE_STOP_LOADING:
        loading = (loading_data_t*)lParam;
        StopLoadingImage(hWnd);
        if (loading && loading->idx != -1)
        {
            MessageBox_(hWnd, IDS_REQUEST_CONTENT_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        }
        if (loading)
            delete loading;
        break;
    case BE_SAVE_FILE:
        WriteOlFile();
        break;
    default:
        break;
    }

    return 0;
}

BOOL OnlineBook::ParserBook(HWND hWnd)
{
    m_IsNotCurnOpenedBook = FALSE;
    if (m_hEvent)
    {
        CloseHandle(m_hEvent);
        m_hEvent = NULL;
    }

    m_result = ReadOlFile();

    if (!m_result)
    {
        return m_result;
    }

    if (m_Chapters.empty())
    {
        m_result = FALSE;
        m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        ParserChapters(hWnd, 0);
        // waitting for request completed.
        WaitForSingleObject(m_hEvent, INFINITE);
    }
    else
    {
        if (!m_Text) // fixed bug
        {
            m_Chapters.clear();
            m_result = FALSE;
            m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            ParserChapters(hWnd, 0);
            // waitting for request completed.
            WaitForSingleObject(m_hEvent, INFINITE);
        }
    }

    return m_result;
}

BOOL OnlineBook::ParserChapterPage(HWND hWnd, int idx)
{
    request_t req;
    req_chapter_param_t* param = NULL;
    req_handler_t hReq;
    std::set<req_handler_t>::iterator it;
    request_t* preq;

    strcpy(m_ChapterPage, m_MainPage);

    if (!m_Booksrc)
        return FALSE;

    if (!m_Booksrc->enable_chapter_page)
        return FALSE;

    memset(m_ChapterPage, 0, sizeof(m_ChapterPage));

    // check it's requesting
    WaitForSingleObject(m_hMutex, INFINITE);
    for (it = m_hRequestList.begin(); it != m_hRequestList.end(); it++)
    {
        preq = hapi_get_request_info(*it);
        if (preq->completer == GetChapterPageCompleter)
        {
            ReleaseMutex(m_hMutex);
            return TRUE;
        }
    }
    ReleaseMutex(m_hMutex);

    param = (req_chapter_param_t*)malloc(sizeof(req_chapter_param_t));

    param->hWnd = hWnd;
    param->index = idx;
    param->_this = this;
    param->title_list = NULL;
    param->title_url = NULL;

    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = m_MainPage;
    req.completer = GetChapterPageCompleter;
    req.param1 = param;
    req.param2 = NULL;

    logger_printk("Request to: %s", req.url);

    hReq = hapi_request(&req);
    if (hReq)
    {
        WaitForSingleObject(m_hMutex, INFINITE);
        m_hRequestList.insert(hReq);
        ReleaseMutex(m_hMutex);
    }
    return TRUE;
}

BOOL OnlineBook::ParserChapters(HWND hWnd, int idx)
{
    request_t req;
    req_chapter_param_t* param = NULL;
    req_handler_t hReq;
    std::set<req_handler_t>::iterator it;
    request_t* preq;

    // parse chapter list page at first
    if (m_ChapterPage[0] == 0)
    {
        if (ParserChapterPage(hWnd, idx))
            return TRUE;
    }

    // check it's requesting
    WaitForSingleObject(m_hMutex, INFINITE);
    for (it = m_hRequestList.begin(); it != m_hRequestList.end(); it++)
    {
        preq = hapi_get_request_info(*it);
        if (preq->completer == GetChaptersCompleter
            /*|| preq->completer == GetChapterPageCompleter*/)
        {
            ReleaseMutex(m_hMutex);
            return TRUE;
        }
    }
    ReleaseMutex(m_hMutex);

    param = (req_chapter_param_t*)malloc(sizeof(req_chapter_param_t));

    param->hWnd = hWnd;
    param->index = idx;
    param->_this = this;
    param->title_list = NULL;
    param->title_url = NULL;

    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = m_ChapterPage; //m_MainPage;
    req.completer = GetChaptersCompleter;
    req.param1 = param;
    req.param2 = NULL;

    logger_printk("Request to: %s", req.url);
    
    hReq = hapi_request(&req);
    if (hReq)
    {
        WaitForSingleObject(m_hMutex, INFINITE);
        m_hRequestList.insert(hReq);
        ReleaseMutex(m_hMutex);
    }
    return TRUE;
}

BOOL OnlineBook::ParserContent(HWND hWnd, int idx, u32 todo)
{
    request_t req;
    req_content_param_t* param = NULL;
    req_handler_t hReq;
    std::set<req_handler_t>::iterator it;
    request_t* preq;
    char url[1024] = { 0 };

    if (idx < 0 || idx >= (int)m_Chapters.size())
        return FALSE;

    // check it's requesting
    WaitForSingleObject(m_hMutex, INFINITE);
    for (it = m_hRequestList.begin(); it != m_hRequestList.end(); it++)
    {
        preq = hapi_get_request_info(*it);
        param = (req_content_param_t*)preq->param1;
        if (preq->completer == GetContentCompleter && param->index == idx)
        {
            // update
            if (param->todo != todo)
                param->todo = todo;
            ReleaseMutex(m_hMutex);
            return TRUE;
        }
    }
    ReleaseMutex(m_hMutex);

    param = (req_content_param_t*)malloc(sizeof(req_content_param_t));

    param->hWnd = hWnd;
    param->index = idx;
    param->todo = todo;
    param->_this = this;
    param->text = NULL;
    param->textlen = 0;

    // check URL
    combine_url(m_Chapters[idx].url.c_str(), m_MainPage, url);

    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = url;
    req.completer = GetContentCompleter;
    req.param1 = param;
    req.param2 = NULL;

    logger_printk("Request to: %s", req.url);

    hReq = hapi_request(&req);
    if (hReq)
    {
        WaitForSingleObject(m_hMutex, INFINITE);
        m_hRequestList.insert(hReq);
        ReleaseMutex(m_hMutex);
    }
    return TRUE;
}

BOOL _check_olfile(const TCHAR *filename)
{
    FILE* fp = NULL;
    char* buf = NULL;
    int len = 0;
    int basesize = 0;
    ol_header_t* header = NULL;
#if 0
    char host[1024] = {0};
#endif

    // read file to memory
    fp = _tfopen(filename, _T("rb"));
    if (!fp)
        goto fail;
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    basesize = sizeof(ol_header_t) - sizeof(ol_chapter_info_t);
    if (basesize > len)
        goto fail;

    buf = (char*)malloc(basesize);
    if (!buf)
        goto fail;

    fread(buf, 1, basesize, fp);
    fclose(fp);
    fp = NULL;

    header = (ol_header_t*)buf;
    if (len < (int)header->header_size)
    {
        // invalid file
        goto fail;
    }

#if 0 // basesize not read host info
    strcpy(host, buf + header->host_offset);
    if (!FindBookSource(host))
    {
        goto fail;
    }
#endif

    return TRUE;

fail:
    if (fp)
        fclose(fp);
    if (buf)
        free(buf);
    return FALSE;
}

BOOL OnlineBook::ReadOlFile(BOOL fast)
{
    FILE* fp = NULL;
    char* buf = NULL;
    int len = 0;
    ol_header_t *header = NULL;
    int basesize = 0;

    // read file to memory
    fp = _tfopen(m_fileName, _T("rb"));
    if (!fp)
        goto fail;
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fast)
    {
        basesize = sizeof(ol_header_t) - sizeof(ol_chapter_info_t);
        if (basesize > len)
            goto fail;

        buf = (char*)malloc(basesize);
        if (!buf)
            goto fail;

        fread(buf, 1, basesize, fp);
        fclose(fp);
        fp = NULL;

        header = (ol_header_t*)buf;
        if (len < (int)header->header_size)
        {
            // invalid file
            goto fail;
        }
        
        m_UpdateTime = header->update_time;
        free(buf);
        return TRUE;
    }

    buf = (char*)malloc(len);
    if (!buf)
        goto fail;

    fread(buf, 1, len, fp);
    fclose(fp);
    fp = NULL;

    // parse ol header
    header = (ol_header_t*)buf;
    if (len < (int)header->header_size)
    {
        // invalid file
        goto fail;
    }
    ParseOlHeader(header);

    // parse book source
    m_Booksrc = FindBookSource(m_Host);
    if (!m_Booksrc)
        goto fail;

    // parse text
    if (m_Chapters.size() > 0 && len > (int)header->header_size)
    {
        m_Length = (len - header->header_size) / 2;
        m_Text = (TCHAR*)malloc((len - header->header_size) + sizeof(TCHAR));
        if (m_Text == NULL)
            goto fail;
        memcpy(m_Text, buf + header->header_size, len - header->header_size);
        m_Text[m_Length] = 0;

    }
    free(buf);
    return TRUE;

fail:
    if (fp)
        fclose(fp);
    if (buf)
        free(buf);
    return FALSE;
}

BOOL OnlineBook::WriteOlFile()
{
    FILE* fp = NULL;
    ol_header_t* header = NULL;

    GenerateOlHeader(&header);
    if (!header)
        goto fail;

    fp = _tfopen(m_fileName, _T("wb"));
    if (!fp)
        goto fail;
    // write header
    fwrite(header, 1, header->header_size, fp);
    // write text
    if (m_Text && m_Length > 0)
        fwrite(m_Text, sizeof(TCHAR), m_Length, fp);
    fclose(fp);
    free(header);
    return TRUE;

fail:
    if (fp)
        fclose(fp);
    if (header)
        free(header);
    return FALSE;
}

BOOL OnlineBook::DownloadPrevNext(HWND hWnd)
{
    int cur = GetCurChapterIndex();
    int prev = cur - 1;
    int next = cur + 1;

    if (cur == -1)
        return FALSE;

    if (m_Chapters.size() == 0)
        return FALSE;

    if (cur < 0 || cur >= (int)m_Chapters.size())
        return FALSE;

    if (next >= 0 && next < (int)m_Chapters.size())
    {
        if (m_Chapters[next].index == -1)
            ParserContent(hWnd, next);
    }

    if (prev >= 0 && prev < (int)m_Chapters.size())
    {
        if (m_Chapters[prev].index == -1)
            ParserContent(hWnd, prev);
    }

    return TRUE;
}

BOOL OnlineBook::OnDrawPageEvent(HWND hWnd)
{
#if TEST_MODEL
    chapters_t::iterator it;
    int last_index = -1;
    int len = 0;
    for (it = m_Chapters.begin(); it != m_Chapters.end(); it++)
    {
        if (it->index != -1)
        {
            ASSERT(it->index >= 0);
            ASSERT(it->index < m_Length);
            ASSERT(wcsncmp(it->title.c_str(), m_Text + it->index, it->title.size()) == 0);
            ASSERT(len == it->index);
            len += it->size;
            if (last_index != -1)
            {
                ASSERT(it->index > last_index);
            }
            else
            {
                ASSERT(it->index == 0);
            }
            last_index = it->index;
        }
    }
    ASSERT(len == m_Length);
#endif
    return DownloadPrevNext(hWnd);
}

BOOL OnlineBook::OnUpDownEvent(HWND hWnd, int draw_type)
{
    int cur = GetCurChapterIndex();
    int prev = cur - 1;
    int next = cur + 1;

    if (cur == -1)
        return FALSE;

    if (m_Chapters.size() == 0)
        return FALSE;

    if (cur < 0 || cur >= (int)m_Chapters.size())
        return FALSE;

    if (draw_type == DRAW_PAGE_UP || draw_type == DRAW_LINE_UP)
    {
        if (prev >= 0 && prev < (int)m_Chapters.size())
        {
            if (m_Chapters[cur].index == m_Index
                && m_Chapters[prev].index == -1)
            {
                m_TagetIndex = prev;
                ParserContent(hWnd, prev, draw_type == DRAW_PAGE_UP ? todo_pageup : todo_lineup);
                PlayLoading(hWnd);
                return FALSE;
            }
        }
    }
    else if (draw_type == DRAW_PAGE_DOWN || draw_type == DRAW_LINE_DOWN)
    {
        if (next >= 0 && next < (int)m_Chapters.size())
        {
            if (m_Index + GetPageLength() == m_Length
                && m_Chapters[next].index == -1)
            {
                m_TagetIndex = next;
                ParserContent(hWnd, next, draw_type == DRAW_PAGE_DOWN ? todo_pagedown : todo_linedown);
                PlayLoading(hWnd);
                return FALSE;
            }
        }
    }
    else
    {
        ASSERT(0);
    }

    return TRUE;
}

void OnlineBook::TidyHtml(char* html, int* len)
{
    char* buf = NULL;
    int index = 0;
    int i;

    buf = (char*)malloc((*len)+1);
    if (!buf)
        return;
    for (i = 0; i < *len; i++)
    {
        // replace "<br><br>"
        if (i < (*len - 8) && strncmp(html+i, "<br><br>", 8) == 0)
        {
            buf[index++] = '\n';
            i += 7;
        }
        else if (i < (*len - 4) && strncmp(html+i, "<br>", 4) == 0)
        {
            buf[index++] = '\n';
            i += 3;
        }
        else
        {
            buf[index++] = html[i];
        }
    }
    if (index < *len)
    {
        *len = index;
        buf[index] = 0;
        memcpy(html, buf, index + 1);
    }
    free(buf);
}

void OnlineBook::FormatHtml(char** html, int* len, int *needfree)
{
    char *htmlfmt = NULL;
    int lenfmt = 0;

    if (*html && (*len) > 0)
    {
        HtmlParser::Instance()->FormatHtml(*html, *len, &htmlfmt, &lenfmt);
        if (htmlfmt && lenfmt > 0)
        {
            if ((*needfree) == 1)
                free(*html);
            *html = htmlfmt;
            *len = lenfmt;
            *needfree = 2;
            //HtmlParser::Instance()->FreeFormat(htmlfmt);
            TidyHtml(*html, len);
        }
    }
}

void OnlineBook::TidyUrl(char* html, int* len)
{
    char* buf = NULL;
    int index = 0;
    int i;

    buf = (char*)malloc((*len) + 1);
    if (!buf)
        return;
    for (i = 0; i < *len; i++)
    {
        // replace "/?"
        if (i < (*len - 2) && html[i] == '/' && html[i + 1] == '?')
        {
            buf[index++] = '?';
            i += 1;
        }
        // replace "&amp;"
        else if (i < (*len - 5) && html[i] == '&' && html[i + 1] == 'a' && html[i + 2] == 'm' && html[i + 3] == 'p' && html[i + 4] == ';')
        {
            buf[index++] = '&';
            i += 4;
        }
        else
        {
            buf[index++] = html[i];
        }
    }
    *len = index;
    buf[index] = 0;
    memcpy(html, buf, index + 1);
    free(buf);
}

void OnlineBook::PlayLoading(HWND hWnd)
{
    if (!m_IsLoading)
    {
        m_IsLoading = TRUE;
        PostMessage(hWnd, WM_BOOK_EVENT, BE_PLAY_LOADING, NULL);
    }
}

void OnlineBook::StopLoading(HWND hWnd, int idx)
{
    loading_data_t* ld = NULL;
    if (m_IsLoading)
    {
        if (m_TagetIndex >= 0 && m_TagetIndex < (int)m_Chapters.size() && m_Chapters[m_TagetIndex].index != -1)
        {
            m_IsLoading = FALSE;
            m_TagetIndex = -1;
            ld = new loading_data_t;
            ld->_this = this;
            ld->idx = -1;
            PostMessage(hWnd, WM_BOOK_EVENT, BE_STOP_LOADING, (LPARAM)ld);
        }
        else if (m_TagetIndex >= 0 && m_TagetIndex < (int)m_Chapters.size() && idx == m_TagetIndex && m_Chapters[m_TagetIndex].index == -1)
        {
            // request error
            m_IsLoading = FALSE;
            m_TagetIndex = -1;
            ld = new loading_data_t;
            ld->_this = this;
            ld->idx = idx;
            PostMessage(hWnd, WM_BOOK_EVENT, BE_STOP_LOADING, (LPARAM)ld);
        }
        else if (m_TagetIndex == -1 && idx == -1) // for manual check update
        {
            m_IsLoading = FALSE;
            m_TagetIndex = -1;
            PostMessage(hWnd, WM_BOOK_EVENT, BE_STOP_LOADING, NULL);
        }
    }
}

BOOL OnlineBook::RequestNextPage(OnlineBook *_this, request_t *r, const char *url, req_handler_t hOld)
{
    req_handler_t hReq = NULL;
    request_t req;
    char url_[1024] = { 0 };

    if (!url)
        return FALSE;

    combine_url(url, r->url, url_);

    if (r->content)
        logger_printk("Redirect request to: %s -> %s", url_, r->content);
    else
        logger_printk("Redirect request to: %s", url_);

    memset(&req, 0, sizeof(request_t));
    req.method = r->method;
    req.url = url_;
    req.content = r->content;
    req.content_length = r->content_length;
    req.completer = r->completer;
    req.param1 = r->param1;
    req.param2 = r->param2;

    hReq = hapi_request(&req);

    if (hReq && m_hMutex)
    {
        WaitForSingleObject(m_hMutex, INFINITE);
        m_hRequestList.erase(hOld);
        m_hRequestList.insert(hReq);
        ReleaseMutex(m_hMutex);
    }
    return hReq != NULL;
}

int OnlineBook::FilterContent(TCHAR* text, int *len)
{
    int srclen = *len;
    int dstlen = 0;
    int i,found = 0,kwlen = 0;
    int offset = 0;
    TCHAR *dsttext = NULL;
    std::wcmatch cm;
    std::wregex* e = NULL;

    if (m_Booksrc->content_filter_type == 1) // filter by keyword
    {
        dsttext = (TCHAR*)malloc(sizeof(TCHAR) * (srclen + 1));
        memset(dsttext, 0, sizeof(TCHAR) * (srclen + 1));
        kwlen = (int)_tcslen(m_Booksrc->content_filter_keyword);
        for (i=0; i<srclen; i++)
        {
            if (srclen-i > kwlen)
            {
                if (_tcsncmp(text + i, m_Booksrc->content_filter_keyword, kwlen) == 0)
                {
                    found = 1;
                    i += kwlen-1; // for loop will i++, so kwlen-1
                    continue;
                }
            }
            dsttext[dstlen++] = text[i];
        }
        if (found)
        {
            memcpy(text, dsttext, sizeof(TCHAR) * (dstlen + 1));
            *len = dstlen;
            if (dstlen == 0)
            {
                text[0] =_T('\n');
                *len = 1;
            }
        }
    }
    else if (m_Booksrc->content_filter_type == 2) // filter by regex
    {
        try
        {
            e = new std::wregex(m_Booksrc->content_filter_keyword);
        }
        catch (...)
        {
            if (e)
            {
                delete e;
            }
            return found;
        }
        dsttext = (TCHAR*)malloc(sizeof(TCHAR) * (srclen + 1));
        memset(dsttext, 0, sizeof(TCHAR) * (srclen + 1));

        while (std::regex_search(text + offset, cm, *e, std::regex_constants::format_first_only))
        {
            found = 1;
            if (cm.position() > 0)
            {
                memcpy(dsttext + dstlen, text + offset, sizeof(TCHAR) * cm.position());
                dstlen += (int)cm.position();
            }
            offset += (int)(cm.position() + cm.length());
        }
        if (e)
        {
            delete e;
        }
        if (found)
        {
            if (srclen > offset)
            {
                memcpy(dsttext + dstlen, text + offset, sizeof(TCHAR) * (srclen - offset));
                dstlen += (srclen - offset);
            }
            memcpy(text, dsttext, sizeof(TCHAR) * (dstlen + 1));
            *len = dstlen;
            if (dstlen == 0)
            {
                text[0] = _T('\n');
                *len = 1;
            }
        }
    }
    else // not do filter
    {
    }
    if (dsttext)
    {
        free(dsttext);
    }
    return found;
}

BOOL OnlineBook::GenerateOlHeader(ol_header_t** header)
{
    int buf_size = 0;
    int offset = 0;
    int i;
    ol_header_t* header_ = NULL;
    char* buf = NULL;

    int base_size = sizeof(ol_header_t) + (sizeof(ol_chapter_info_t) * ((int)m_Chapters.size() - 1));
    int bookname_size = ((int)_tcslen(m_BookName) + 1) * sizeof(TCHAR);
    int mainpage_size = ((int)strlen(m_MainPage) + 1) * sizeof(char);
    int host_size = ((int)strlen(m_Host) + 1) * sizeof(char);

    // calc buf size
    buf_size += base_size;
    buf_size += bookname_size;
    buf_size += mainpage_size;
    buf_size += host_size;
    for (i = 0; i < (int)m_Chapters.size(); i++)
    {
        buf_size += ((int)m_Chapters[i].title.size() + 1) * sizeof(TCHAR);
        buf_size += ((int)m_Chapters[i].url.size() + 1) * sizeof(char);
    }

    // set offset
    header_ = (ol_header_t*)malloc(buf_size);
    if (!header_)
        return FALSE;
    header_->header_size = buf_size;
    offset = base_size;
    header_->book_name_offset = offset;
    offset += bookname_size;
    header_->main_page_offset = offset;
    offset += mainpage_size;
    header_->host_offset = offset;
    offset += host_size;
    header_->update_time = m_UpdateTime;
    // header_->is_finished = m_IsFinished; deprecated
    header_->chapter_size = (int)m_Chapters.size();
    for (i = 0; i < (int)m_Chapters.size(); i++)
    {
        header_->chapter_info_list[i].index = m_Chapters[i].index;
        header_->chapter_info_list[i].size = m_Chapters[i].size;
        header_->chapter_info_list[i].title_offset = offset;
        offset += ((int)m_Chapters[i].title.size() + 1) * sizeof(TCHAR);
        header_->chapter_info_list[i].url_offset = offset;
        offset += ((int)m_Chapters[i].url.size() + 1) * sizeof(char);
    }

    // set data
    buf = (char*)header_;
    memcpy(buf + header_->book_name_offset, m_BookName, bookname_size);
    memcpy(buf + header_->main_page_offset, m_MainPage, mainpage_size);
    memcpy(buf + header_->host_offset, m_Host, host_size);
    for (i = 0; i < (int)m_Chapters.size(); i++)
    {
        memcpy(buf + header_->chapter_info_list[i].title_offset, m_Chapters[i].title.c_str(), (m_Chapters[i].title.size() + 1) * sizeof(TCHAR));
        memcpy(buf + header_->chapter_info_list[i].url_offset, m_Chapters[i].url.c_str(), (m_Chapters[i].url.size() + 1) * sizeof(char));
    }

    *header = header_;
    return TRUE;
}

BOOL OnlineBook::ParseOlHeader(ol_header_t* header)
{
    int chapter_size = (int)header->chapter_size;
    chapter_item_t item;
    char* buf = (char*)header;
    int i;

    _tcscpy(m_BookName, (TCHAR*)(buf + header->book_name_offset));
    strcpy(m_MainPage, buf + header->main_page_offset);
    strcpy(m_Host, buf + header->host_offset);
    m_UpdateTime = header->update_time;

    m_Chapters.clear();
    for (i = 0; i < chapter_size; i++)
    {
        ol_chapter_info_t* cinfo = &(header->chapter_info_list[i]);
        item.index = cinfo->index;
        item.size = cinfo->size;
        item.title = (TCHAR*)(buf + cinfo->title_offset);
        item.url = buf + cinfo->url_offset;
        item.title_len = cinfo->title_offset/2;
        m_Chapters.push_back(item);
    }

    return TRUE;
}

unsigned int OnlineBook::GetChapterPageCompleter(request_result_t *result)
{
    req_chapter_param_t* param = (req_chapter_param_t*)result->param1;
    OnlineBook* _this = (OnlineBook*)param->_this;
    char* html = NULL;
    int htmllen = 0;
    std::vector<std::string> chapter_url;
    void* doc = NULL;
    void* ctx = NULL;
    int needfree = 0;
    int ret = 1;

    check_request_result(result);

    HtmlParser::Instance()->HtmlParseBegin(html, htmllen, &doc, &ctx, &_this->m_bForceKill);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->chapter_page_xpath, chapter_url, &_this->m_bForceKill);
    HtmlParser::Instance()->HtmlParseEnd(doc, ctx);

    if (_this->m_bForceKill)
        goto end;

    if (chapter_url.size() == 0)
    {
        DumpParseErrorFile(html, htmllen);
        goto end;
    }

    // save chapter list page url
    combine_url(chapter_url[0].c_str(), result->req->url, _this->m_ChapterPage);

    // parser chapters
    _this->ParserChapters(param->hWnd, param->index);

    ret = 0;

end:
    if (needfree && html)
        free(html);
    if (!result->cancel)
    {
        if (ret && _this->m_hEvent)
            SetEvent(_this->m_hEvent);
        if (_this->m_hMutex)
        {
            WaitForSingleObject(_this->m_hMutex, INFINITE);
            if (_this->m_hRequestList.find(result->handler) != _this->m_hRequestList.end())
                _this->m_hRequestList.erase(result->handler);
            ReleaseMutex(_this->m_hMutex);
        }
    }
    if (param)
    {
        // <-- for manual check update
        if (param->index == -1 && _this->m_IsLoading && ret != 0)
        {
            _this->StopLoading(param->hWnd, -1);
        }
        // -->
        free(param);
    }
    if (ret != 0)
    {
        if (_this->m_cb)
            _this->m_cb(FALSE, ret, _this->m_arg);
    }
    return ret;
}

unsigned int OnlineBook::GetChaptersCompleter(request_result_t *result)
{
    req_chapter_param_t* param = (req_chapter_param_t*)result->param1;
    OnlineBook* _this = (OnlineBook*)param->_this;
    char* html = NULL;
    int htmllen = 0;
    std::vector<std::string> title_list;
    std::vector<std::string> title_url;
    std::vector<std::string> url_xpath;
    std::vector<std::string> keyword_xpath;
    void* doc = NULL;
    void* ctx = NULL;
    int i;
    chapter_data_t chapters;
    chapter_item_t item;
    TCHAR* dst = NULL;
    int dstlen;
    int needfree = 0;
    int ret = 1;
    char dsturl[1024];

    check_request_result(result);

    HtmlParser::Instance()->HtmlParseBegin(html, htmllen, &doc, &ctx, &_this->m_bForceKill);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->chapter_title_xpath, title_list, &_this->m_bForceKill);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->chapter_url_xpath, title_url, &_this->m_bForceKill);
    if (_this->m_Booksrc->enable_chapter_next)
    {
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->chapter_next_url_xpath, url_xpath, &_this->m_bForceKill, TRUE);
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->chapter_next_keyword_xpath, keyword_xpath, &_this->m_bForceKill, TRUE);
    }
    HtmlParser::Instance()->HtmlParseEnd(doc, ctx);

    if (_this->m_bForceKill)
        goto end;

    if (title_list.size() == 0 || title_list.size() != title_url.size())
    {
        DumpParseErrorFile(html, htmllen);
        goto end;
    }

    if (_this->m_Booksrc->enable_chapter_next)
    {
        // save data
        if (param->title_url == NULL)
        {
            param->title_url = new std::vector<std::string>;
            param->title_list = new std::vector<std::string>;
        }

        param->title_url->insert(param->title_url->end(), title_url.begin(), title_url.end());
        param->title_list->insert(param->title_list->end(), title_list.begin(), title_list.end());

        if (!url_xpath.empty() && !keyword_xpath.empty())
        {
            if (strstr(_this->m_Booksrc->chapter_next_keyword, keyword_xpath[0].c_str())) // exist next content
            {
                // request next content
                if (_this->RequestNextPage(_this, result->req, url_xpath[0].c_str(), result->handler))
                    goto _next;
            }
        }

        // update chapter
        chapters._this = _this;
        for (i = 0; i < (int)param->title_url->size(); i++)
        {
            if (_this->m_bForceKill)
                goto end;

            // format title
            dst = Utf8ToUtf16(param->title_list->at(i).c_str());
            dstlen = (int)_tcslen(dst);
            _this->FormatText(dst, &dstlen);

            combine_url(param->title_url->at(i).c_str(), result->req->url, dsturl);

            item.index = -1;
            item.size = 0;
            item.title = dst;
            item.url = dsturl;
            item.title_len = dstlen;
            chapters.chapters.push_back(item);
        }
    }
    else
    {
        // update chapter
        chapters._this = _this;
        for (i = 0; i < (int)title_url.size(); i++)
        {
            if (_this->m_bForceKill)
                goto end;

            // format title
            dst = Utf8ToUtf16(title_list[i].c_str());
            dstlen = (int)_tcslen(dst);
            _this->FormatText(dst, &dstlen);

            combine_url(title_url[i].c_str(), result->req->url, dsturl);

            item.index = -1;
            item.size = 0;
            item.title = dst;
            item.url = dsturl;
            item.title_len = dstlen;
            chapters.chapters.push_back(item);
        }
    }
    _this->m_UpdateTime = time(NULL);
    SendMessage(param->hWnd, WM_BOOK_EVENT, BE_UPATE_CHAPTER, (LPARAM)&chapters);

    if (_this->m_bForceKill)
        goto end;

    // parser content
    if (param->index == -1)
    {
        chapters.ret = 1;
    }
    else
    {
        _this->ParserContent(param->hWnd, param->index);
    }
    ret = 0;

end:
    if (needfree && html)
        free(html);
    if (!result->cancel)
    {
        if (ret && _this->m_hEvent)
            SetEvent(_this->m_hEvent);
        if (_this->m_hMutex)
        {
            WaitForSingleObject(_this->m_hMutex, INFINITE);
            if (_this->m_hRequestList.find(result->handler) != _this->m_hRequestList.end())
                _this->m_hRequestList.erase(result->handler);
            ReleaseMutex(_this->m_hMutex);
        }
    }
    if (param)
    {
        // <-- for manual check update
        if (param->index == -1 && _this->m_IsLoading)
        {
            _this->StopLoading(param->hWnd, -1);
        }
        // -->

        if (param->title_url)
            delete param->title_url;
        if (param->title_list)
            delete param->title_list;
        free(param);
    }
    if (chapters.ret != 0)
    {
        if (_this->m_cb)
            _this->m_cb(chapters.is_updated, ret, _this->m_arg);
    }
    return ret;

_next:
    if (needfree && html)
        free(html);
    return 1;   
}

unsigned int OnlineBook::GetContentCompleter(request_result_t *result)
{
    req_content_param_t* param = (req_content_param_t*)result->param1;
    OnlineBook* _this = (OnlineBook*)param->_this;
    char* html = NULL;
    int htmllen = 0;
    std::vector<std::string> content_list;
    std::vector<std::string> url_xpath;
    std::vector<std::string> keyword_xpath;
    content_data_t data;
    void* doc = NULL;
    void* ctx = NULL;
    TCHAR* dst = NULL;
    int dstlen;
    int needfree = 0;
    int ret = 1;

    check_request_result(result);

    _this->FormatHtml(&html, &htmllen, &needfree);

    if (_this->m_bForceKill)
        goto end;

    HtmlParser::Instance()->HtmlParseBegin(html, htmllen, &doc, &ctx, &_this->m_bForceKill);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->content_xpath, content_list, &_this->m_bForceKill);
    if (_this->m_Booksrc->enable_content_next)
    {
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->content_next_url_xpath, url_xpath, &_this->m_bForceKill, TRUE);
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->content_next_keyword_xpath, keyword_xpath, &_this->m_bForceKill, TRUE);
    }
    HtmlParser::Instance()->HtmlParseEnd(doc, ctx);
    
    if (_this->m_bForceKill)
        goto end;

    if (content_list.size() == 0 || content_list[0].size() == 0)
    {
        DumpParseErrorFile(html, htmllen);
        goto end;
    }

    // insert title
    if (!param || param->index < 0 || param->index >= (int)_this->m_Chapters.size())
        goto end;

    if (_this->m_Booksrc->enable_content_next)
    {
        if (param->text == NULL)
        {
            content_list[0].insert(0, "\n");
            content_list[0].insert(0, Utf16ToUtf8(_this->m_Chapters[param->index].title.c_str()));
            content_list[0].append("\n");
        }
    }
    else
    {
        content_list[0].insert(0, "\n");
        content_list[0].insert(0, Utf16ToUtf8(_this->m_Chapters[param->index].title.c_str()));
        content_list[0].append("\n");
    }

    if (_this->m_bForceKill)
        goto end;

    // format content
    dst = utf8_to_utf16(content_list[0].c_str(), (int)content_list[0].size(), &dstlen);
    if (!dst)
        goto end;

    if (_this->m_bForceKill)
        goto end;

    _this->FormatText(dst, &dstlen);

    if (_this->m_bForceKill)
        goto end;

    if (_this->FilterContent(dst, &dstlen))
    {
        _this->FormatText(dst, &dstlen);
    }

    if (_this->m_bForceKill)
        goto end;

    if (_this->m_Booksrc->enable_content_next)
    {
        // save data
        if (param->text == NULL)
        {
            param->textlen = dstlen;
            param->text = dst;
            param->text[param->textlen] = 0;
            dst = NULL;
        }
        else
        {
            param->textlen += dstlen;
            param->text = (TCHAR*)realloc(param->text, sizeof(TCHAR)*(param->textlen+1));
            memcpy(param->text+(param->textlen-dstlen), dst, sizeof(TCHAR)*dstlen);
            param->text[param->textlen] = 0;
        }

        if (!url_xpath.empty() && !keyword_xpath.empty())
        {
            if (strstr(_this->m_Booksrc->content_next_keyword, keyword_xpath[0].c_str())) // exist next content
            {
                // request next content
                if (_this->RequestNextPage(_this, result->req, url_xpath[0].c_str(), result->handler))
                    goto _next;
            }
        }

        data._this = _this;
        data.index = param->index;
        data.text = param->text;
        data.len = param->textlen;
        data.todo = param->todo;
    }
    else
    {
        data._this = _this;
        data.index = param->index;
        data.text = dst;
        data.len = dstlen;
        data.todo = param->todo;
    }

    SendMessage(param->hWnd, WM_BOOK_EVENT, BE_UPATE_CONTENT, (LPARAM)&data);

    _this->m_result = TRUE;
    ret = 0;

end:
    if (html)
        if (needfree == 1)
            free(html);
        else if (needfree == 2)
            HtmlParser::Instance()->FreeFormat(html);
    if (dst)
        free(dst);
    if (!result->cancel)
    {
        if (_this->m_hEvent)
            SetEvent(_this->m_hEvent);
        if (_this->m_hMutex)
        {
            WaitForSingleObject(_this->m_hMutex, INFINITE);
            if (_this->m_hRequestList.find(result->handler) != _this->m_hRequestList.end())
                _this->m_hRequestList.erase(result->handler);
            ReleaseMutex(_this->m_hMutex);
        }
        if (param)
        {
            _this->StopLoading(param->hWnd, param->index);
            if (param->text)
                free(param->text);
            free(param);
        }
    }
    return ret;

_next:
    if (html)
        if (needfree == 1)
            free(html);
        else if (needfree == 2)
            HtmlParser::Instance()->FreeFormat(html);
    if (dst)
        free(dst);
    return 1;
}

void OnlineBook::UpdateBookSource(void)
{
    m_Booksrc = FindBookSource(m_Host);
}

int OnlineBook::CheckUpdate(HWND hWnd, olbook_checkupdate_callback cb, void* arg)
{
    u64 current_time = 0;

    logger_printk("file=%s", Utf16ToAnsi(m_fileName));

    if (m_IsNotCurnOpenedBook)
    {
        if (!ReadOlFile(TRUE))
            return 1; // fail
    }

    current_time = time(NULL);
    if (current_time <= m_UpdateTime || current_time - m_UpdateTime < 4 * 3600)
        return 0; // completed

    if (m_IsNotCurnOpenedBook)
    {
        if (!ReadOlFile())
            return 1; // fail
    }
    
    m_cb = cb;
    m_arg = arg;

    // get chapters
    ParserChapters(hWnd, -1);

    return 2; // do check
}

int OnlineBook::ManualCheckUpdate(HWND hWnd, olbook_checkupdate_callback cb, void* arg)
{
    m_cb = cb;
    m_arg = arg;

    // get chapters
    ParserChapters(hWnd, -1);

    PlayLoading(hWnd);

    return 2; // do check
}

#endif
