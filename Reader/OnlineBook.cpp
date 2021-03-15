#include "stdafx.h"
#ifdef ENABLE_NETWORK
#include "OnlineBook.h"
#include "Utils.h"
#include "resource.h"
#include <time.h>
#include <regex>
#include <shellapi.h>
#if TEST_MODEL
#include <assert.h>
#endif

extern bool PlayLoadingImage(HWND);
extern bool StopLoadingImage(HWND);
extern int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType);
extern book_source_t* FindBookSource(const char* host);
extern void DumpParseErrorFile(const char *html, int htmllen);

typedef struct req_chapter_param_t
{
    HWND hWnd;
    int index;
    OnlineBook* _this;
} req_chapter_param_t;

typedef struct req_content_param_t
{
    HWND hWnd;
    int index;
    u32 todo;
    OnlineBook* _this;
} req_content_param_t;

typedef struct req_bookstatus_param_t
{
    HWND hWnd;
    OnlineBook* _this;
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
    , m_result(false)
    , m_IsLoading(FALSE)
    , m_TagetIndex(-1)
    , m_Booksrc(NULL)
    , m_cb(NULL)
    , m_arg(NULL)
    , m_IsCheck(TRUE)
{
    memset(m_MainPage, 0, sizeof(m_MainPage));
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
    m_result = false;
}

book_type_t OnlineBook::GetBookType()
{
    return book_online;
}

bool OnlineBook::SaveBook(HWND hWnd)
{
    return false;
}

bool OnlineBook::UpdateChapters(int offset)
{
    return false;
}

bool OnlineBook::IsLoading(void)
{
    return m_hThread != NULL || m_IsLoading;
}

void OnlineBook::JumpChapter(HWND hWnd, int index)
{
    if (index < 0 || index > (int)m_Chapters.size())
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
            (*m_CurrentPos) = m_Chapters[prev].index;
            Reset(hWnd);
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
            (*m_CurrentPos) = m_Chapters[next].index;
            Reset(hWnd);
        }
    }
}

int OnlineBook::GetCurChapterIndex(void)
{
    int index = -1;
    chapters_t::iterator itor;

    if (!m_Text)
        return index;

    if (m_Chapters.size() <= 0)
        return index;

    if (!m_CurrentPos)
        return index;

    itor = m_Chapters.begin();
    index = itor->first;
    for (itor = m_Chapters.begin(); itor != m_Chapters.end(); itor++)
    {
        if (itor->second.index != -1 && itor->second.index > (*m_CurrentPos))
        {
            break;
        }
        if (itor->second.index != -1)
            index = itor->first;
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
            m_Chapters.insert(chapters->chapters.begin(), chapters->chapters.end());
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
#if TEST_MODEL
                assert(m_Chapters[i].title == (chapters->chapters)[i].title);
                assert(m_Chapters[i].url == (chapters->chapters)[i].url);
#endif
                // fixed
                if (m_Chapters[i].index == -1
                    && (m_Chapters[i].title != (chapters->chapters)[i].title || m_Chapters[i].url != (chapters->chapters)[i].url))
                {
                    m_Chapters[i].title = (chapters->chapters)[i].title;
                    m_Chapters[i].url = (chapters->chapters)[i].url;
                }
            }
            for (i = m_Chapters.size(); i < chapters->chapters.size(); i++)
            {
                m_Chapters.insert(std::make_pair(i, (chapters->chapters)[i]));
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

#if TEST_MODEL
        assert(m_Chapters[content->index].index == -1);
#endif

        if (m_Text == NULL) // update text
        {
            m_TextLength = content->len;
            m_Text = (TCHAR*)malloc((m_TextLength + 1) * sizeof(TCHAR));
            memcpy(m_Text, content->text, (m_TextLength + 1) * sizeof(TCHAR));
            // update chapter index
            m_Chapters[content->index].index = 0;
            m_Chapters[content->index].size = content->len;
            ret = 1;
        }
        else // insert text
        {
            m_TextLength += content->len;
            m_Text = (TCHAR*)realloc(m_Text, (m_TextLength + 1) * sizeof(TCHAR));
            m_Text[m_TextLength] = 0;

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
                m_Chapters[content->index].index = m_TextLength - content->len;
                m_Chapters[content->index].size = content->len;
                memcpy(m_Text + m_Chapters[content->index].index, content->text, sizeof(TCHAR) * content->len);
            }
            else // insert
            {
                m_Chapters[content->index].index = offset;
                m_Chapters[content->index].size = content->len;
                memcpy(m_Text + offset + content->len, m_Text + offset, sizeof(TCHAR) * (m_TextLength - offset - content->len));
                memcpy(m_Text + offset, content->text, sizeof(TCHAR) * content->len);
            }

            // update current pos
            if (m_CurrentPos)
            {
                if ((*m_CurrentPos) >= m_Chapters[content->index].index)
                    (*m_CurrentPos) += content->len;
            }
            RemoveAllLine();
            // todo after request completed.
            switch (content->todo & 0xFFFF)
            {
            case todo_jump:
                if (m_CurrentPos)
                {
                    (*m_CurrentPos) = m_Chapters[content->index].index; //  then redraw by stoploading
                }
                break;
            case todo_pageup:
                PageUp(hWnd);  // Won't come here
                break;
            case todo_pagedown:
                PageDown(hWnd); // Won't come here
                break;
            case todo_lineup:
                //LineUp(hWnd, content->todo >> 16); // LineUp will return by IsValid(), so just change m_CurrentLine, then redraw by stoploading
                m_CurrentLine -= content->todo >> 16;
                break;
            case todo_linedown:
                //LineDown(hWnd, content->todo >> 16); // LineDown will return by IsValid(), so just change m_CurrentLine, then redraw by stoploading
                //m_CurrentLine += content->todo >> 16; // If the current page is not full, it is incorrect
                if (m_CurrentPos)
                {
                    (*m_CurrentPos) = m_Chapters[content->index].index;
                }
                break;
            default:
                break;
            }

#if TEST_MODEL
            {
                char msg[256] = { 0 };
                if (m_CurrentPos)
                    sprintf(msg, "--- BE_UPATE_CONTENT: pos=%d, index=%d, size=%d, curpos=%d ---\n", content->index, m_Chapters[content->index].index, m_Chapters[content->index].size, *m_CurrentPos);
                else
                    sprintf(msg, "--- BE_UPATE_CONTENT: pos=%d, index=%d, size=%d ---\n", content->index, m_Chapters[content->index].index, m_Chapters[content->index].size);
                OutputDebugStringA(msg);
            }
#endif
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

bool OnlineBook::ParserBook(HWND hWnd)
{
    m_IsCheck = FALSE;
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
        m_result = false;
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
            m_result = false;
            m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            ParserChapters(hWnd, 0);
            // waitting for request completed.
            WaitForSingleObject(m_hEvent, INFINITE);
        }
    }

    return m_result;
}

bool OnlineBook::ParserChapters(HWND hWnd, int idx)
{
    request_t req;
    req_chapter_param_t* param = NULL;
    req_handler_t hReq;
    std::set<req_handler_t>::iterator it;
    request_t* preq;
#if TEST_MODEL
    char msg[1024];
#endif

    // check it's requesting
    WaitForSingleObject(m_hMutex, INFINITE);
    for (it = m_hRequestList.begin(); it != m_hRequestList.end(); it++)
    {
        preq = hapi_get_request_info(*it);
        if (preq->completer == GetChaptersCompleter)
        {
            ReleaseMutex(m_hMutex);
            return true;
        }
    }
    ReleaseMutex(m_hMutex);

    param = (req_chapter_param_t*)malloc(sizeof(req_chapter_param_t));

    param->hWnd = hWnd;
    param->index = idx;
    param->_this = this;

    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = m_MainPage;
    req.completer = GetChaptersCompleter;
    req.param1 = param;
    req.param2 = NULL;

#if TEST_MODEL
    sprintf(msg, "Request to: %s\n", req.url);
    OutputDebugStringA(msg);
#endif
    
    hReq = hapi_request(&req);
    if (hReq)
    {
        WaitForSingleObject(m_hMutex, INFINITE);
        m_hRequestList.insert(hReq);
        ReleaseMutex(m_hMutex);
    }
    return true;
}

bool OnlineBook::ParserContent(HWND hWnd, int idx, u32 todo)
{
    request_t req;
    req_content_param_t* param = NULL;
    req_handler_t hReq;
    std::set<req_handler_t>::iterator it;
    request_t* preq;
    char url[1024] = { 0 };
#if TEST_MODEL
    char msg[1024];
#endif

    if (idx == -1 || (int)m_Chapters.size() <= idx)
        return false;

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
            return true;
        }
    }
    ReleaseMutex(m_hMutex);

    param = (req_content_param_t*)malloc(sizeof(req_content_param_t));

    param->hWnd = hWnd;
    param->index = idx;
    param->todo = todo;
    param->_this = this;

    // check URL
    if (_strnicmp("http", m_Chapters[idx].url.c_str(), 4) == 0)
    {
        strcpy(url, m_Chapters[idx].url.c_str());
    }
    else
    {
        char* bookid = strstr(m_MainPage, m_Host);
        if (bookid)
        {
            bookid += strlen(m_Host);
            if (strstr(m_Chapters[idx].url.c_str(), bookid))
            {
                strcpy(url, m_Host);
                strcat(url, m_Chapters[idx].url.c_str());
            }
            else
            {
                strcpy(url, m_MainPage);
                strcat(url, m_Chapters[idx].url.c_str());
            }
        }
    }

    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = url;
    req.completer = GetContentCompleter;
    req.param1 = param;
    req.param2 = NULL;

#if TEST_MODEL
    sprintf(msg, "Request to: %s\n", req.url);
    OutputDebugStringA(msg);
#endif

    hReq = hapi_request(&req);
    if (hReq)
    {
        WaitForSingleObject(m_hMutex, INFINITE);
        m_hRequestList.insert(hReq);
        ReleaseMutex(m_hMutex);
    }
    return true;
}

bool OnlineBook::ParserBookStatus(HWND hWnd)
{
    request_t req;
    req_bookstatus_param_t* param = NULL;
    req_handler_t hReq;
    std::set<req_handler_t>::iterator it;
    request_t* preq;
    char url[1024] = { 0 };
    char* encode;
#if TEST_MODEL
    char msg[1024];
#endif

    // check it's requesting
    WaitForSingleObject(m_hMutex, INFINITE);
    for (it = m_hRequestList.begin(); it != m_hRequestList.end(); it++)
    {
        preq = hapi_get_request_info(*it);
        if (preq->completer == GetBookStatusCompleter)
        {
            ReleaseMutex(m_hMutex);
            return true;
        }
    }
    ReleaseMutex(m_hMutex);

    param = (req_bookstatus_param_t*)malloc(sizeof(req_bookstatus_param_t));

    param->hWnd = hWnd;
    param->_this = this;

    Utils::UrlEncode(Utils::Utf16ToUtf8(m_BookName), &encode);
    sprintf(url, m_Booksrc->query_url, encode);
    Utils::UrlFree(encode);

    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = url;
    req.completer = GetBookStatusCompleter;
    req.param1 = param;
    req.param2 = NULL;

#if TEST_MODEL
    sprintf(msg, "Request to: %s\n", req.url);
    OutputDebugStringA(msg);
#endif

    hReq = hapi_request(&req);
    if (hReq)
    {
        WaitForSingleObject(m_hMutex, INFINITE);
        m_hRequestList.insert(hReq);
        ReleaseMutex(m_hMutex);
    }
    return true;
}

bool _check_olfile(const TCHAR *filename)
{
    FILE* fp = NULL;
    char* buf = NULL;
    int len = 0;
    int basesize = 0;
    ol_header_t* header = NULL;
    char host[1024] = {0};

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

    return true;

fail:
    if (fp)
        fclose(fp);
    if (buf)
        free(buf);
    return false;
}

bool OnlineBook::ReadOlFile(BOOL fast)
{
    FILE* fp = NULL;
    char* buf = NULL;
    int len = 0;
    bool result = false;
    char mainpage[1024] = { 0 };
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

        m_IsFinished = header->is_finished;
        m_UpdateTime = header->update_time;
        free(buf);
        return true;
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
        m_TextLength = (len - header->header_size) / 2;
        m_Text = (TCHAR*)malloc((len - header->header_size) + sizeof(TCHAR));
        if (m_Text == NULL)
            goto fail;
        memcpy(m_Text, buf + header->header_size, len - header->header_size);
        m_Text[m_TextLength] = 0;

    }
    free(buf);
    return true;

fail:
    if (fp)
        fclose(fp);
    if (buf)
        free(buf);
    return false;
}

bool OnlineBook::WriteOlFile()
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
    if (m_Text && m_TextLength > 0)
        fwrite(m_Text, sizeof(TCHAR), m_TextLength, fp);
    fclose(fp);
    free(header);
    return true;

fail:
    if (fp)
        fclose(fp);
    if (header)
        free(header);
    return false;
}

bool OnlineBook::DownloadPrevNext(HWND hWnd)
{
    int cur = GetCurChapterIndex();
    int prev = cur - 1;
    int next = cur + 1;

    if (cur == -1)
        return false;

    if (m_Chapters.size() == 0)
        return false;

    if (cur < 0 || cur >= (int)m_Chapters.size())
        return false;

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

    return true;
}

bool OnlineBook::OnDrawPageEvent(HWND hWnd)
{
#if TEST_MODEL
    chapters_t::iterator it;
    int last_index = -1;
    int len = 0;
    for (it = m_Chapters.begin(); it != m_Chapters.end(); it++)
    {
        if (it->second.index != -1)
        {
            assert(it->second.index >= 0);
            assert(it->second.index < m_TextLength);
            assert(wcsncmp(it->second.title.c_str(), m_Text + it->second.index, it->second.title.size()) == 0);
            assert(len == it->second.index);
            len += it->second.size;
            if (last_index != -1)
            {
                assert(it->second.index > last_index);
            }
            else
            {
                assert(it->second.index == 0);
            }
            last_index = it->second.index;
        }
    }
    assert(len == m_TextLength);
#endif
    return DownloadPrevNext(hWnd);
}

bool OnlineBook::OnLineUpDownEvent(HWND hWnd, BOOL up, int n)
{
    int cur = GetCurChapterIndex();
    int prev = cur - 1;
    int next = cur + 1;

    if (cur == -1)
        return false;

    if (m_Chapters.size() == 0)
        return false;

    if (cur < 0 || cur >= (int)m_Chapters.size())
        return false;

    if (up)
    {
        if (prev >= 0 && prev < (int)m_Chapters.size())
        {
            if (m_Chapters[cur].index == *m_CurrentPos
                && m_Chapters[prev].index == -1)
            {
                m_TagetIndex = prev;
                ParserContent(hWnd, prev, n << 16 | todo_lineup);
                PlayLoading(hWnd);
                return false;
            }
            else if (m_Chapters[prev].index != -1 && m_PageInfo.line_size <= 0) // fixed cannot line up bug
            {
                m_CurrentLine = 0;
                ReDraw(hWnd);
                return false;
            }
        }
    }
    else
    {
        if (next >= 0 && next < (int)m_Chapters.size())
        {
            if ((*m_CurrentPos) + m_CurPageSize == m_TextLength
                && m_Chapters[next].index == -1)
            {
                m_TagetIndex = next;
                ParserContent(hWnd, next, n << 16 | todo_linedown);
                PlayLoading(hWnd);
                return false;
            }
            else if (m_CurrentPos && m_Chapters[next].index != -1 && m_Chapters[next].index == m_CurPageSize+(*m_CurrentPos)) // fixed not full page bug
            {
                *m_CurrentPos = m_Chapters[next].index;
                Reset(hWnd);
                return false;
            }
        }
    }

    return true;
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
        if (m_TagetIndex != -1 && m_Chapters[m_TagetIndex].index != -1)
        {
            m_IsLoading = FALSE;
            m_TagetIndex = -1;
            ld = new loading_data_t;
            ld->_this = this;
            ld->idx = -1;
            PostMessage(hWnd, WM_BOOK_EVENT, BE_STOP_LOADING, (LPARAM)ld);
        }
        else if (m_TagetIndex != -1 && idx == m_TagetIndex && m_Chapters[m_TagetIndex].index == -1)
        {
            // request error
            m_IsLoading = FALSE;
            m_TagetIndex = -1;
            ld = new loading_data_t;
            ld->_this = this;
            ld->idx = idx;
            PostMessage(hWnd, WM_BOOK_EVENT, BE_STOP_LOADING, (LPARAM)ld);
        }
    }
}

BOOL OnlineBook::Redirect(OnlineBook *_this, request_t *r, const char *url, req_handler_t hOld)
{
    request_t req;
    req_handler_t hReq;
    char url_[1024] = {0};
#if TEST_MODEL
    char msg[1024] = { 0 };
#endif

    if (!url)
        return FALSE;

#if TEST_MODEL
    if (r->content)
        sprintf(msg, "Redirect request to: %s -> %s\n", url, r->content);
    else
        sprintf(msg, "Redirect request to: %s\n", url);
    OutputDebugStringA(msg);
#endif

    if (_strnicmp("http", url, 4) != 0)
    {
        sprintf(url_, "%s%s", _this->m_Host, url);
    }
    else
    {
        strcpy(url_, url);
    }

    memset(&req, 0, sizeof(request_t));
    req.method = r->method;
    req.url = url_;
    req.content = r->content;
    req.content_length = r->content_length;
    req.completer = r->completer;
    req.param1 = r->param1;
    req.param2 = r->param2;

    hReq = hapi_request(&req);
    if (m_hMutex)
    {
        WaitForSingleObject(m_hMutex, INFINITE);
        m_hRequestList.erase(hOld);
        m_hRequestList.insert(hReq);
        ReleaseMutex(m_hMutex);
    }
    return TRUE;
}

bool OnlineBook::GenerateOlHeader(ol_header_t** header)
{
    int buf_size = 0;
    int offset = 0;
    int i;
    ol_header_t* header_ = NULL;
    char* buf = NULL;

    int base_size = sizeof(ol_header_t) + (sizeof(ol_chapter_info_t) * ((int)m_Chapters.size() - 1));
    int bookname_size = (_tcslen(m_BookName) + 1) * sizeof(TCHAR);
    int mainpage_size = (strlen(m_MainPage) + 1) * sizeof(char);
    int host_size = (strlen(m_Host) + 1) * sizeof(char);

    // calc buf size
    buf_size += base_size;
    buf_size += bookname_size;
    buf_size += mainpage_size;
    buf_size += host_size;
    for (i = 0; i < (int)m_Chapters.size(); i++)
    {
        buf_size += (m_Chapters[i].title.size() + 1) * sizeof(TCHAR);
        buf_size += (m_Chapters[i].url.size() + 1) * sizeof(char);
    }

    // set offset
    header_ = (ol_header_t*)malloc(buf_size);
    if (!header_)
        return false;
    header_->header_size = buf_size;
    offset = base_size;
    header_->book_name_offset = offset;
    offset += bookname_size;
    header_->main_page_offset = offset;
    offset += mainpage_size;
    header_->host_offset = offset;
    offset += host_size;
    header_->update_time = m_UpdateTime;
    header_->is_finished = m_IsFinished;
    header_->chapter_size = m_Chapters.size();
    for (i = 0; i < (int)m_Chapters.size(); i++)
    {
        header_->chapter_info_list[i].index = m_Chapters[i].index;
        header_->chapter_info_list[i].size = m_Chapters[i].size;
        header_->chapter_info_list[i].title_offset = offset;
        offset += (m_Chapters[i].title.size() + 1) * sizeof(TCHAR);
        header_->chapter_info_list[i].url_offset = offset;
        offset += (m_Chapters[i].url.size() + 1) * sizeof(char);
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
    return true;
}

bool OnlineBook::ParseOlHeader(ol_header_t* header)
{
    int chapter_size = (int)header->chapter_size;
    chapter_item_t item;
    char* buf = (char*)header;
    int i;

    _tcscpy(m_BookName, (TCHAR*)(buf + header->book_name_offset));
    strcpy(m_MainPage, buf + header->main_page_offset);
    strcpy(m_Host, buf + header->host_offset);
    m_UpdateTime = header->update_time;
    m_IsFinished = header->is_finished;

    for (i = 0; i < chapter_size; i++)
    {
        ol_chapter_info_t* cinfo = &(header->chapter_info_list[i]);
        item.index = cinfo->index;
        item.size = cinfo->size;
        item.title = (TCHAR*)(buf + cinfo->title_offset);
        item.url = buf + cinfo->url_offset;
        m_Chapters.insert(std::make_pair(i, item));
    }

    return true;
}

unsigned int OnlineBook::GetChaptersCompleter(request_result_t *result)
{
    req_chapter_param_t* param = (req_chapter_param_t*)result->param1;
    OnlineBook* _this = (OnlineBook*)param->_this;
    char* html = NULL;
    int htmllen = 0;
    std::vector<std::string> title_list;
    std::vector<std::string> title_url;
    std::vector<std::string> book_status;
    void* doc = NULL;
    void* ctx = NULL;
    int i;
    chapter_data_t chapters;
    chapter_item_t item;
    TCHAR* dst = NULL;
    int dstlen;
    int needfree = 0;
    int ret = 1;

    if (result->cancel)
        goto end;

    if (result->errno_ != succ)
        goto end;

    if (result->status_code != 200)
    {
        // redirect
        if (_this->Redirect(_this, result->req, hapi_get_location(result->header), result->handler))
        {
            // update m_MainPage
            const char *url = hapi_get_location(result->header);
            if (_strnicmp("http", url, 4) != 0)
            {
                sprintf(_this->m_MainPage, "%s%s", _this->m_Host, url);
            }
            else
            {
                strcpy(_this->m_MainPage, url);
            }
            return 1;
        }   
        goto end;
    }

    if (hapi_is_gzip(result->header))
    {
        needfree = 1;
        if (!Utils::gzipInflate((unsigned char*)result->body, result->bodylen, (unsigned char**)&html, &htmllen))
        {
            needfree = 0;
            goto end;
        }
    }
    else
    {
        html = result->body;
        htmllen = result->bodylen;
    }

#if 0
    //if (hapi_get_charset(result->header) != utf_8)
#else
    if (!Utils::is_utf8(html, htmllen)) // fixed bug, focus check encode
#endif
    {
        // conver to gbk
        wchar_t* tempbuf = NULL;
        int templen = 0;
        char* utf8buf = NULL;
        int utf8len = 0;
        // convert 'gbk' to 'utf-8'
        tempbuf = Utils::ansi_to_utf16_ex(html, htmllen, &templen);
        utf8buf = Utils::utf16_to_utf8_ex(tempbuf, templen, &utf8len);
        free(tempbuf);
        if (needfree)
        {
            free(html);
        }
        html = utf8buf;
        htmllen = utf8len;
        needfree = 1;
    }

    if (_this->m_bForceKill)
        goto end;

    HtmlParser::Instance()->HtmlParseBegin(html, htmllen, &doc, &ctx, &_this->m_bForceKill);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->chapter_title_xpath, title_list, &_this->m_bForceKill);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->chapter_url_xpath, title_url, &_this->m_bForceKill);
    if (_this->m_Booksrc->book_status_pos == 2)
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->book_status_xpath, book_status, &_this->m_bForceKill, true);
    HtmlParser::Instance()->HtmlParseEnd(doc, ctx);

    if (_this->m_bForceKill)
        goto end;

    if (title_list.size() == 0 || title_list.size() != title_url.size())
    {
        DumpParseErrorFile(html, htmllen);
        goto end;
    }

    if (_this->m_Booksrc->book_status_pos == 2)
    {
        if (!book_status.empty())
        {
            if (strcmp(book_status[0].c_str(), _this->m_Booksrc->book_status_keyword) == 0)
            {
                _this->m_IsFinished = TRUE;
            }
        }
    }

    // update chapter
    chapters._this = _this;
    for (i = 0; i < (int)title_url.size(); i++)
    {
        if (_this->m_bForceKill)
            goto end;

        // format title
        dst = Utils::Utf8ToUtf16(title_list[i].c_str());
        dstlen = _tcslen(dst);
        _this->FormatText(dst, &dstlen);

        item.index = -1;
        item.size = 0;
        item.title = dst;
        item.url = title_url[i];
        chapters.chapters.insert(std::make_pair(i, item));
    }
    _this->m_UpdateTime = time(NULL);
    SendMessage(param->hWnd, WM_BOOK_EVENT, BE_UPATE_CHAPTER, (LPARAM)&chapters);

    if (_this->m_bForceKill)
        goto end;

    // parser content
    if (param->index == -1)
    {
        if (chapters.ret == 0 && !_this->m_IsFinished)
        {
            if (_this->m_Booksrc->book_status_pos == 1)
                _this->ParserBookStatus(param->hWnd);
            else
                chapters.ret = 1;
        }
    }
    else
    {
        _this->ParserContent(param->hWnd, param->index);
    }
    ret = 0;

end:
    if (param)
        free(param);
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
    if (ret == 1 || chapters.ret != 0)
    {
        if (_this->m_cb)
            _this->m_cb(FALSE, ret, _this->m_arg);
    }
    return ret;
}

unsigned int OnlineBook::GetContentCompleter(request_result_t *result)
{
    req_content_param_t* param = (req_content_param_t*)result->param1;
    OnlineBook* _this = (OnlineBook*)param->_this;
    char* html = NULL;
    int htmllen = 0;
    std::vector<std::string> content_list;
    content_data_t data;
    TCHAR* dst = NULL;
    int dstlen;
    int needfree = 0;
    int ret = 1;

    if (result->cancel)
        goto end;

    if (result->errno_ != succ)
        goto end;

    if (result->status_code != 200)
    {
        // redirect
        if (_this->Redirect(_this, result->req, hapi_get_location(result->header), result->handler))
            return 1;
        goto end;
    }

    if (hapi_is_gzip(result->header))
    {
        needfree = 1;
        if (!Utils::gzipInflate((unsigned char*)result->body, result->bodylen, (unsigned char**)&html, &htmllen))
        {
            needfree = 0;
            goto end;
        }
    }
    else
    {
        html = result->body;
        htmllen = result->bodylen;
    }

#if 0
    //if (hapi_get_charset(result->header) != utf_8)
#else
    if (!Utils::is_utf8(html, htmllen)) // fixed bug, focus check encode
#endif
    {
        // conver to gbk
        wchar_t* tempbuf = NULL;
        int templen = 0;
        char* utf8buf = NULL;
        int utf8len = 0;
        // convert 'gbk' to 'utf-8'
        tempbuf = Utils::ansi_to_utf16_ex(html, htmllen, &templen);
        utf8buf = Utils::utf16_to_utf8_ex(tempbuf, templen, &utf8len);
        free(tempbuf);
        if (needfree)
        {
            free(html);
        }
        html = utf8buf;
        htmllen = utf8len;
        needfree = 1;
    }

    if (_this->m_bForceKill)
        goto end;

    _this->FormatHtml(&html, &htmllen, &needfree);

    if (_this->m_bForceKill)
        goto end;

    HtmlParser::Instance()->HtmlParseByXpath(html, htmllen, _this->m_Booksrc->content_xpath, content_list, &_this->m_bForceKill);
    
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

    content_list[0].insert(0, "\n");
    content_list[0].insert(0, Utils::Utf16ToUtf8(_this->m_Chapters[param->index].title.c_str()));
    content_list[0].append("\n");

    if (_this->m_bForceKill)
        goto end;

    // format content
    dst = Utils::utf8_to_utf16_ex(content_list[0].c_str(), content_list[0].size(), &dstlen);
    if (!dst)
        goto end;
    
    if (_this->m_bForceKill)
        goto end;

    _this->FormatText(dst, &dstlen);

    if (_this->m_bForceKill)
        goto end;

    data._this = _this;
    data.index = param->index;
    data.text = dst;
    data.len = dstlen;
    data.todo = param->todo;
    SendMessage(param->hWnd, WM_BOOK_EVENT, BE_UPATE_CONTENT, (LPARAM)&data);

    _this->m_result = true;
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
            _this->StopLoading(param->hWnd, param->index);
        if (param)
            free(param);
    }
    return ret;
}

unsigned int OnlineBook::GetBookStatusCompleter(request_result_t *result)
{
    req_bookstatus_param_t* param = (req_bookstatus_param_t*)result->param1;
    OnlineBook* _this = (OnlineBook*)param->_this;
    char* html = NULL;
    int htmllen = 0;
    int i;
    std::vector<std::string> table_name;
    std::vector<std::string> table_url;
    std::vector<std::string> table_author;
    std::vector<std::string> table_status;
    void* doc = NULL;
    void* ctx = NULL;
    book_event_data_t data;
    int needfree = 0;
    int ret = 1;
    char Url[1024] = { 0 };

    if (result->cancel)
        goto end;

    if (result->errno_ != succ)
        goto end;

    if (result->status_code != 200)
    {
        // redirect
        if (_this->Redirect(_this, result->req, hapi_get_location(result->header), result->handler))
            return 1;
        goto end;
    }

    if (hapi_is_gzip(result->header))
    {
        needfree = 1;
        if (!Utils::gzipInflate((unsigned char*)result->body, result->bodylen, (unsigned char**)&html, &htmllen))
        {
            needfree = 0;
            goto end;
        }
    }
    else
    {
        html = result->body;
        htmllen = result->bodylen;
    }

#if 0
    //if (hapi_get_charset(result->header) != utf_8)
#else
    if (!Utils::is_utf8(html, htmllen)) // fixed bug, focus check encode
#endif
    {
        // conver to gbk
        wchar_t* tempbuf = NULL;
        int templen = 0;
        char* utf8buf = NULL;
        int utf8len = 0;
        // convert 'gbk' to 'utf-8'
        tempbuf = Utils::ansi_to_utf16_ex(html, htmllen, &templen);
        utf8buf = Utils::utf16_to_utf8_ex(tempbuf, templen, &utf8len);
        free(tempbuf);
        if (needfree)
        {
            free(html);
        }
        html = utf8buf;
        htmllen = utf8len;
        needfree = 1;
    }

    if (_this->m_bForceKill)
        goto end;

    HtmlParser::Instance()->HtmlParseBegin(html, htmllen, &doc, &ctx, &_this->m_bForceKill);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->book_name_xpath, table_name, &_this->m_bForceKill);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->book_mainpage_xpath, table_url, &_this->m_bForceKill);
    if (_this->m_Booksrc->book_author_xpath[0])
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->book_author_xpath, table_author, &_this->m_bForceKill);
    if (_this->m_Booksrc->book_status_pos == 1)
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _this->m_Booksrc->book_status_xpath, table_status, &_this->m_bForceKill);
    HtmlParser::Instance()->HtmlParseEnd(doc, ctx);

    if (_this->m_bForceKill)
        goto end;

    if (table_url.empty() || table_url.size() != table_name.size())
        goto end;

    if (table_url.size() == table_status.size())
    {
        for (i = 0; i < (int)table_url.size(); i++)
        {
            if (_strnicmp("http", table_url[i].c_str(), 4) != 0)
            {
                sprintf(Url, "%s%s", _this->m_Host, table_url[i].c_str());
            }
            if (strcmp(_this->m_MainPage, Url) == 0)
            {
                if (strcmp(table_status[i].c_str(), _this->m_Booksrc->book_status_keyword) == 0)
                {
                    _this->m_IsFinished = TRUE;
                }
                data._this = _this;
                SendMessage(param->hWnd, WM_BOOK_EVENT, BE_SAVE_FILE, (LPARAM)&data);
                break;
            }
        }
    }

end:
    if (param)
        free(param);
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
    if (_this->m_cb)
        _this->m_cb(TRUE, 0, _this->m_arg);
    return ret;
}

void OnlineBook::UpdateBookSource(void)
{
    m_Booksrc = FindBookSource(m_Host);
}

int OnlineBook::CheckUpdate(HWND hWnd, olbook_checkupdate_callback cb, void* arg)
{
    u64 current_time = 0;
#if TEST_MODEL
    char msg[1024] = { 0 };
    char* ansi = NULL;
#endif

#if TEST_MODEL
    ansi = Utils::Utf16ToAnsi(m_fileName);
    sprintf(msg, "{%s:%d} file=%s\n", __FUNCTION__, __LINE__, ansi);
    OutputDebugStringA(msg);
#endif

    if (m_IsCheck)
    {
        if (!ReadOlFile(TRUE))
            return 1; // fail
    }

    if (m_IsFinished)
        return 0; // completed

    current_time = time(NULL);
    if (current_time <= m_UpdateTime || current_time - m_UpdateTime < 4 * 3600)
        return 0; // completed

    if (m_IsCheck)
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

#endif
