#ifdef ENABLE_NETWORK

/*
 * WebBook.cpp - 通过局域网 web 服务(legado/开源阅读)读书，并与手机同步进度
 *
 * 网络层用 WinHTTP（系统自带 winhttp.dll），不用工程里的 libhttps：
 * 这个版本的 libhttps 无法处理带端口的 URL（http://host:80/ 直接 errno=-1），
 * 而 legado web 服务就在 host:1122 上，所以必须换。
 *
 * 线程约定：
 *   - 打开阶段 ParserBook 跑在 Book::OpenBook 的工作线程里，直接用同步 HTTP；
 *   - 打开之后换章/回写进度用自己的工作线程做同步 HTTP，结果用 SendMessage(WM_BOOK_EVENT)
 *     送回界面线程；工作线程只持有自己的堆参数副本，不碰书对象；
 *   - 只有界面线程改 m_Text / m_Chapters / m_Index 并重绘。
 */

#include "WebBook.h"
#include "resource.h"
#include "types.h"
#include "Utils.h"
#include "cJSON.h"

#include <winhttp.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

extern void OnOpenBook(HWND hWnd, TCHAR *filename, BOOL forced);
extern BOOL PlayLoadingImage(HWND hWnd);
extern BOOL StopLoadingImage(HWND hWnd);
extern int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType);
extern Book* _Book;

#define WEB_FILE_SAVE_PATH      _T(".web\\")
#define WEB_HTTP_TIMEOUT_MS     15000
#define WEB_POST_TIMEOUT_MS     5000
#define WEB_SYNC_INTERVAL_MS    5000

// ---------------------------------------------------------------------------
// 小工具
// ---------------------------------------------------------------------------

static long long NowMs(void)
{
    FILETIME ft;
    long long t;

    GetSystemTimeAsFileTime(&ft);
    t = ((long long)ft.dwHighDateTime << 32) | (long long)ft.dwLowDateTime;
    return t / 10000 - 11644473600000LL;    // 100ns -> ms, 1601 -> 1970
}

static std::string Utf8UrlEncode(const std::string& s)
{
    static const char* hex = "0123456789ABCDEF";
    std::string ret;
    size_t i;

    for (i = 0; i < s.size(); i++)
    {
        unsigned char c = (unsigned char)s[i];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || c == '-' || c == '_' || c == '.' || c == '~')
        {
            ret += (char)c;
        }
        else
        {
            ret += '%';
            ret += hex[c >> 4];
            ret += hex[c & 0x0F];
        }
    }
    return ret;
}

static std::string WideToUtf8(const std::wstring& w)
{
    char* buf = NULL;
    int len = 0;
    std::string ret;

    if (w.empty())
        return ret;
    buf = utf16_to_utf8(w.c_str(), (int)w.size(), &len);
    if (buf)
    {
        ret.assign(buf, len);
        free_buffer(buf);
    }
    return ret;
}

static std::wstring Utf8ToWide(const char* s)
{
    wchar_t* buf = NULL;
    int len = 0;
    std::wstring ret;

    if (!s || !s[0])
        return ret;
    buf = utf8_to_utf16(s, (int)strlen(s), &len);
    if (buf)
    {
        ret.assign(buf, len);
        free_buffer(buf);
    }
    return ret;
}

static std::string JsonStr(cJSON* obj, const char* key)
{
    cJSON* item = obj ? cJSON_GetObjectItem(obj, key) : NULL;

    if (item && cJSON_IsString(item) && item->valuestring)
        return item->valuestring;
    return "";
}

static int JsonInt(cJSON* obj, const char* key, int def = 0)
{
    cJSON* item = obj ? cJSON_GetObjectItem(obj, key) : NULL;

    if (item && cJSON_IsNumber(item))
        return (int)item->valuedouble;
    return def;
}

static long long JsonLL(cJSON* obj, const char* key, long long def = 0)
{
    cJSON* item = obj ? cJSON_GetObjectItem(obj, key) : NULL;

    if (item && cJSON_IsNumber(item))
        return (long long)item->valuedouble;
    return def;
}

// ---------------------------------------------------------------------------
// WinHTTP 同步请求（可在任意工作线程调用；不碰 Reader 全局状态）
// ---------------------------------------------------------------------------

static HINTERNET s_hSession = NULL;
static INIT_ONCE s_sessionOnce = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK WebSessionInit(PINIT_ONCE once, PVOID param, PVOID* ctx)
{
    // NO_PROXY：这是局域网服务，绝不能走系统代理
    s_hSession = WinHttpOpen(L"Reader-WebRead/2.0", WINHTTP_ACCESS_TYPE_NO_PROXY,
                             WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    return TRUE;
}

int WebHttpRequest(const char* method, const char* url, const char* body, int bodylen,
                   char** out_body, int* out_len, int timeout_ms)
{
    URL_COMPONENTS uc;
    WCHAR host[256] = { 0 };
    WCHAR path[4096] = { 0 };
    WCHAR extra[2048] = { 0 };
    WCHAR wmethod[16] = { 0 };
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    std::wstring wurl;
    std::wstring target;
    char* data = NULL;
    int total = 0;
    int cap = 0;
    int status = 0;

    if (out_body)
        *out_body = NULL;
    if (out_len)
        *out_len = 0;
    if (!url || !url[0])
        return 0;

    InitOnceExecuteOnce(&s_sessionOnce, WebSessionInit, NULL, NULL);
    if (!s_hSession)
        return 0;

    wurl = Utf8ToWide(url);
    if (wurl.empty())
        return 0;

    memset(&uc, 0, sizeof(uc));
    uc.dwStructSize = sizeof(uc);
    uc.lpszHostName = host;
    uc.dwHostNameLength = 255;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 4095;
    uc.lpszExtraInfo = extra;
    uc.dwExtraInfoLength = 2047;

    if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.size(), 0, &uc))
        return 0;

    target = path;
    target += extra;    // 查询串

    _snwprintf(wmethod, 15, L"%S", method ? method : "GET");

    hConnect = WinHttpConnect(s_hSession, host, uc.nPort, 0);
    if (!hConnect)
        goto end;

    hRequest = WinHttpOpenRequest(hConnect, wmethod, target.c_str(), NULL, WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES,
                                  uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest)
        goto end;

    WinHttpSetTimeouts(hRequest, timeout_ms, timeout_ms, timeout_ms, timeout_ms);

#ifdef WINHTTP_OPTION_DECOMPRESSION
    {
        DWORD decomp = WINHTTP_DECOMPRESSION_FLAG_ALL;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_DECOMPRESSION, &decomp, sizeof(decomp));
    }
#endif

    if (!WinHttpSendRequest(hRequest,
                            L"Content-Type: application/json; charset=utf-8\r\n",
                            (DWORD)-1L,
                            (LPVOID)((body && bodylen > 0) ? body : NULL),
                            (DWORD)bodylen, (DWORD)bodylen, 0))
    {
        goto end;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL))
        goto end;

    {
        DWORD code = 0;
        DWORD size = sizeof(code);
        if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                 WINHTTP_HEADER_NAME_BY_INDEX, &code, &size, WINHTTP_NO_HEADER_INDEX))
        {
            goto end;
        }
        status = (int)code;
    }

    for (;;)
    {
        DWORD avail = 0;
        DWORD read = 0;

        if (!WinHttpQueryDataAvailable(hRequest, &avail))
            break;
        if (avail == 0)
            break;

        if (total + (int)avail + 1 > cap)
        {
            char* p = (char*)realloc(data, total + (int)avail + 4096);
            if (!p)
                break;
            data = p;
            cap = total + (int)avail + 4096;
        }

        if (!WinHttpReadData(hRequest, data + total, avail, &read))
            break;
        if (read == 0)
            break;
        total += (int)read;
    }

    if (data)
        data[total] = 0;

end:
    if (hRequest)
        WinHttpCloseHandle(hRequest);
    if (hConnect)
        WinHttpCloseHandle(hConnect);

    if (status == 200 && data)
    {
        if (out_body)
            *out_body = data;
        else
            free(data);
        if (out_len)
            *out_len = total;
    }
    else
    {
        if (data)
            free(data);
    }

    return status;
}

// ---------------------------------------------------------------------------
// 换章：后台线程取正文，结果用 SendMessage 送回界面线程
// ---------------------------------------------------------------------------

typedef struct web_fetch_task_t
{
    HWND    hWnd;
    Book*   book;           // 只做路由用，工作线程不解引用
    char*   url;            // 自己的一份拷贝
    int     seq;
    int     chapter_index;
    int     pos;
} web_fetch_task_t;

typedef struct web_content_result_t
{
    Book*    _this;         // 必须是第一个成员：Reader 用 be->_this 判断事件属于哪本书
    int      seq;
    int      chapter_index;
    int      pos;
    wchar_t* text;          // 所有权交给 OnBookEvent
    int      len;
} web_content_result_t;

static unsigned __stdcall WebFetchThread(void* param)
{
    web_fetch_task_t* task = (web_fetch_task_t*)param;
    web_content_result_t res;
    char* body = NULL;
    int len = 0;
    int status;
    HWND hWnd;

    memset(&res, 0, sizeof(res));
    if (!task)
        return 0;

    status = WebHttpRequest("GET", task->url, NULL, 0, &body, &len, WEB_HTTP_TIMEOUT_MS);

    if (status == 200 && body)
    {
        cJSON* root = cJSON_Parse(body);
        if (root)
        {
            cJSON* ok = cJSON_GetObjectItem(root, "isSuccess");
            cJSON* data = cJSON_GetObjectItem(root, "data");
            if (cJSON_IsTrue(ok) && cJSON_IsString(data) && data->valuestring && data->valuestring[0])
                res.text = utf8_to_utf16(data->valuestring, (int)strlen(data->valuestring), &res.len);
            cJSON_Delete(root);
        }
        free(body);
    }

    res._this = task->book;
    res.seq = task->seq;
    res.chapter_index = task->chapter_index;
    res.pos = task->pos;

    hWnd = task->hWnd;
    free(task->url);
    free(task);

    if (hWnd)
        SendMessage(hWnd, WM_BOOK_EVENT, res.text ? WEB_BE_CONTENT : WEB_BE_CONTENT_FAIL, (LPARAM)&res);

    if (res.text)   // 界面线程没接手（书已切换/已关闭）就自己释放
        free(res.text);
    return 0;
}

// ---------------------------------------------------------------------------
// 回写进度：后台线程发 POST，自己收尾
// ---------------------------------------------------------------------------

typedef struct web_post_task_t
{
    char* url;
    char* body;
    int   bodylen;
} web_post_task_t;

static unsigned __stdcall WebPostThread(void* param)
{
    web_post_task_t* task = (web_post_task_t*)param;
    char* body = NULL;
    int len = 0;

    if (!task)
        return 0;

    WebHttpRequest("POST", task->url, task->body, task->bodylen, &body, &len, WEB_POST_TIMEOUT_MS);
    if (body)
        free(body);
    free(task->url);
    free(task->body);
    free(task);
    return 0;
}

// ---------------------------------------------------------------------------
// .web 暂存文件（书籍身份 + 上次阅读位置）
// ---------------------------------------------------------------------------

static void FillMetaFromJson(cJSON* root, web_book_meta_t* meta)
{
    if (!root || !meta)
        return;

    meta->server = JsonStr(root, "server");
    meta->book_url = JsonStr(root, "bookUrl");
    meta->name = JsonStr(root, "name");
    meta->author = JsonStr(root, "author");
    meta->chapter_title = JsonStr(root, "chapterTitle");
    meta->chapter_index = JsonInt(root, "chapterIndex", 0);
    meta->chapter_pos = JsonInt(root, "chapterPos", 0);
    meta->chapter_time = JsonLL(root, "chapterTime", 0);
    meta->fetch_time = JsonLL(root, "fetchTime", 0);
}

static BOOL ReadMetaFile(const TCHAR* path, web_book_meta_t* meta)
{
    FILE* fp = NULL;
    char* buf = NULL;
    long size;
    cJSON* root = NULL;
    BOOL ret = FALSE;

    fp = _tfopen(path, _T("rb"));
    if (!fp)
        return FALSE;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size <= 0 || size > 65536)
    {
        fclose(fp);
        return FALSE;
    }

    buf = (char*)malloc(size + 1);
    if (!buf)
    {
        fclose(fp);
        return FALSE;
    }
    if ((long)fread(buf, 1, size, fp) != size)
    {
        fclose(fp);
        free(buf);
        return FALSE;
    }
    buf[size] = 0;
    fclose(fp);

    root = cJSON_Parse(buf);
    if (root)
    {
        FillMetaFromJson(root, meta);
        cJSON_Delete(root);
        ret = meta->book_url.empty() ? FALSE : TRUE;
    }
    free(buf);
    return ret;
}

static BOOL WriteMetaFile(const TCHAR* path, const web_book_meta_t* meta)
{
    cJSON* root = NULL;
    char* json = NULL;
    FILE* fp = NULL;
    BOOL ret = FALSE;

    if (!path || !meta)
        return FALSE;

    root = cJSON_CreateObject();
    if (!root)
        return FALSE;

    cJSON_AddStringToObject(root, "server", meta->server.c_str());
    cJSON_AddStringToObject(root, "bookUrl", meta->book_url.c_str());
    cJSON_AddStringToObject(root, "name", meta->name.c_str());
    cJSON_AddStringToObject(root, "author", meta->author.c_str());
    cJSON_AddStringToObject(root, "chapterTitle", meta->chapter_title.c_str());
    cJSON_AddNumberToObject(root, "chapterIndex", (double)meta->chapter_index);
    cJSON_AddNumberToObject(root, "chapterPos", (double)meta->chapter_pos);
    cJSON_AddNumberToObject(root, "chapterTime", (double)meta->chapter_time);
    cJSON_AddNumberToObject(root, "fetchTime", (double)meta->fetch_time);

    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json)
        return FALSE;

    fp = _tfopen(path, _T("wb"));
    if (fp)
    {
        fwrite(json, 1, strlen(json), fp);
        fclose(fp);
        ret = TRUE;
    }
    free(json);
    return ret;
}

// ---------------------------------------------------------------------------
// WebBook
// ---------------------------------------------------------------------------

// 节流窗口内发生的改动，用线程定时器在窗口结束后补推一次
static UINT_PTR  s_SyncTimer = 0;
static WebBook*  s_SyncBook = NULL;

static void CALLBACK WebSyncTimerProc(HWND hWnd, UINT message, UINT_PTR idEvent, DWORD dwTime)
{
    KillTimer(NULL, idEvent);
    if (s_SyncTimer == idEvent)
        s_SyncTimer = 0;
    if (s_SyncBook)
        s_SyncBook->FlushProgress();
}


WebBook::WebBook()
    : m_CurIndex(0)
    , m_PendingPos(-1)
    , m_PosInChapter(0)
    , m_SavedPos(-1)
    , m_SavedIndex(-1)
    , m_LastSyncTick(0)
    , m_IsLoading(FALSE)
    , m_FetchSeq(0)
    , m_FetchingIndex(-1)
{
    m_Meta.chapter_index = 0;
    m_Meta.chapter_pos = 0;
    m_Meta.chapter_time = 0;
    m_Meta.fetch_time = 0;
}

WebBook::~WebBook()
{
    if (s_SyncBook == this)
    {
        if (s_SyncTimer)
        {
            KillTimer(NULL, s_SyncTimer);
            s_SyncTimer = 0;
        }
        s_SyncBook = NULL;
    }
    m_FetchSeq++;               // 让在途请求的结果作废
    SyncProgress(NULL, TRUE);   // 退出/换书时把进度写回本地文件并同步给手机
    ForceKill();
}

book_type_t WebBook::GetBookType(void)
{
    return book_web;
}

BOOL WebBook::IsLoading(void)
{
    return m_IsLoading || Book::IsLoading();
}

BOOL WebBook::SaveBook(HWND hWnd)
{
    // 正文在手机上，不需要本地保存
    return FALSE;
}

BOOL WebBook::UpdateChapters(int offset)
{
    return FALSE;
}

int WebBook::GetCurChapterIndex(void)
{
    if (m_CurIndex < 0 || m_CurIndex >= (int)m_Chapters.size())
        return -1;
    return m_CurIndex;
}

std::string WebBook::BaseUrl(const char* api) const
{
    return m_Meta.server + api;
}

std::string WebBook::ContentUrl(int index) const
{
    char tmp[32];

    sprintf(tmp, "%d", index);
    return BaseUrl("/getBookContent?url=") + Utf8UrlEncode(m_Meta.book_url) + "&index=" + tmp;
}

BOOL WebBook::LoadMeta(void)
{
    if (!m_fileName[0])
        return FALSE;
    if (!ReadMetaFile(m_fileName, &m_Meta))
        return FALSE;
    if (m_Meta.server.empty() || m_Meta.book_url.empty())
        return FALSE;

    while (!m_Meta.server.empty() && m_Meta.server[m_Meta.server.size() - 1] == '/')
        m_Meta.server.erase(m_Meta.server.size() - 1);

    return TRUE;
}

BOOL WebBook::SaveMeta(void)
{
    if (!m_fileName[0])
        return FALSE;
    return WriteMetaFile(m_fileName, &m_Meta);
}

// GET /getBookshelf，取出这本书在手机上的进度；比本地新就用手机的
void WebBook::RefreshProgressFromServer(void)
{
    char* body = NULL;
    int len = 0;
    int status;
    cJSON* root = NULL;
    cJSON* list = NULL;
    int i, count;

    status = WebHttpRequest("GET", BaseUrl("/getBookshelf").c_str(), NULL, 0, &body, &len, WEB_HTTP_TIMEOUT_MS);
    if (status != 200 || !body)
    {
        if (body)
            free(body);
        return;
    }

    root = cJSON_Parse(body);
    if (root)
    {
        list = cJSON_GetObjectItem(root, "data");
        count = cJSON_IsArray(list) ? cJSON_GetArraySize(list) : 0;
        for (i = 0; i < count && !m_bForceKill; i++)
        {
            cJSON* item = cJSON_GetArrayItem(list, i);
            if (!item)
                continue;
            if (m_Meta.book_url != JsonStr(item, "bookUrl"))
                continue;

            if (m_Meta.name.empty())
                m_Meta.name = JsonStr(item, "name");
            if (m_Meta.author.empty())
                m_Meta.author = JsonStr(item, "author");

            {
                long long t = JsonLL(item, "durChapterTime", 0);
                if (t > m_Meta.chapter_time)
                {
                    m_Meta.chapter_time = t;
                    m_Meta.chapter_index = JsonInt(item, "durChapterIndex", 0);
                    m_Meta.chapter_pos = JsonInt(item, "durChapterPos", 0);
                    m_Meta.chapter_title = JsonStr(item, "durChapterTitle");
                }
            }
            break;
        }
        cJSON_Delete(root);
    }
    free(body);

    m_Meta.fetch_time = NowMs();
    SaveMeta();
}

BOOL WebBook::GetChapterList(void)
{
    char* body = NULL;
    int len = 0;
    int status;
    cJSON* root = NULL;
    cJSON* list = NULL;
    int i, count;

    status = WebHttpRequest("GET", (BaseUrl("/getChapterList?url=") + Utf8UrlEncode(m_Meta.book_url)).c_str(),
                            NULL, 0, &body, &len, WEB_HTTP_TIMEOUT_MS);
    if (status != 200 || !body)
    {
        if (body)
            free(body);
        return FALSE;
    }

    root = cJSON_Parse(body);
    if (!root)
    {
        free(body);
        return FALSE;
    }

    list = cJSON_GetObjectItem(root, "data");
    count = cJSON_IsArray(list) ? cJSON_GetArraySize(list) : 0;

    m_Chapters.clear();
    for (i = 0; i < count; i++)
    {
        cJSON* item = cJSON_GetArrayItem(list, i);
        chapter_item_t chapter;

        if (!item)
            continue;

        chapter.index = -1;         // -1: 未加载正文（WebBook 一次只在内存里放当前章）
        chapter.title = Utf8ToWide(JsonStr(item, "title").c_str());
        chapter.url = JsonStr(item, "url");
        chapter.size = 0;
        chapter.title_len = 0;
        m_Chapters.push_back(chapter);
    }

    cJSON_Delete(root);
    free(body);

    return m_Chapters.empty() ? FALSE : TRUE;
}

wchar_t* WebBook::GetChapterContent(int index, int* len)
{
    char* body = NULL;
    int bodylen = 0;
    int status;
    cJSON* root = NULL;
    wchar_t* text = NULL;

    if (len)
        *len = 0;

    status = WebHttpRequest("GET", ContentUrl(index).c_str(), NULL, 0, &body, &bodylen, WEB_HTTP_TIMEOUT_MS);
    if (status != 200 || !body)
    {
        if (body)
            free(body);
        return NULL;
    }

    root = cJSON_Parse(body);
    if (root)
    {
        cJSON* ok = cJSON_GetObjectItem(root, "isSuccess");
        cJSON* data = cJSON_GetObjectItem(root, "data");
        if (cJSON_IsTrue(ok) && cJSON_IsString(data) && data->valuestring && data->valuestring[0])
        {
            int wlen = 0;
            text = utf8_to_utf16(data->valuestring, (int)strlen(data->valuestring), &wlen);
            if (text && len)
                *len = wlen;
        }
        cJSON_Delete(root);
    }
    free(body);

    return text;
}

BOOL WebBook::ParserBook(HWND hWnd)
{
    wchar_t* text = NULL;
    int len = 0;
    int index, pos;
    int i;

    if (!LoadMeta())
        return FALSE;

    // 打开的是阅读列表里的旧书时，先问手机要最新进度（书架刚拉过就跳过，避免每次开书都拉一次大 JSON）
    if (NowMs() - m_Meta.fetch_time > 60000)
        RefreshProgressFromServer();

    if (m_bForceKill)
        return FALSE;

    if (!GetChapterList())
        return FALSE;

    if (m_bForceKill)
        return FALSE;

    index = m_Meta.chapter_index;
    if (index < 0 || index >= (int)m_Chapters.size())
        index = 0;
    pos = m_Meta.chapter_pos;
    if (pos < 0)
        pos = 0;

    text = GetChapterContent(index, &len);
    if (!text || len <= 0)
    {
        if (text)
            free(text);
        return FALSE;
    }

    if (m_bForceKill)
    {
        free(text);
        return FALSE;
    }

    m_Text = text;
    m_Length = len;
    m_CurIndex = index;
    m_PendingPos = pos > len ? len : pos;
    m_PosInChapter = m_PendingPos;
    if (index < (int)m_Chapters.size())
        m_Meta.chapter_title = WideToUtf8(m_Chapters[index].title);

    // 一次只加载一章：当前章 offset=0，其余 -1（与 OnlineBook 的“未下载”约定一致）
    for (i = 0; i < (int)m_Chapters.size(); i++)
        m_Chapters[i].index = (i == index) ? 0 : -1;

    return TRUE;
}

BOOL WebBook::ApplyChapter(HWND hWnd, int index, int pos, wchar_t* text, int len)
{
    int i;

    if (!text || len <= 0)
        return FALSE;

    if (m_Text)
        free(m_Text);
    m_Text = text;
    m_Length = len;
    m_CurIndex = index;
    m_PendingPos = (pos < 0 || pos > len) ? 0 : pos;
    m_PosInChapter = m_PendingPos;

    for (i = 0; i < (int)m_Chapters.size(); i++)
        m_Chapters[i].index = (i == index) ? 0 : -1;

    m_Meta.chapter_index = index;
    m_Meta.chapter_pos = m_PendingPos;
    if (index >= 0 && index < (int)m_Chapters.size())
        m_Meta.chapter_title = WideToUtf8(m_Chapters[index].title);

    SyncProgress(hWnd, TRUE);
    PostMessage(hWnd, WM_UPDATE_CHAPTERS, 0, NULL);   // 左侧目录重建
    ReDraw(hWnd);
    return TRUE;
}

void WebBook::RequestChapter(HWND hWnd, int index, int pos)
{
    web_fetch_task_t* task;
    unsigned thread_id;
    std::string url;

    if (index < 0 || index >= (int)m_Chapters.size())
        return;

    url = ContentUrl(index);

    task = (web_fetch_task_t*)calloc(1, sizeof(web_fetch_task_t));
    if (!task)
        return;
    task->hWnd = hWnd;
    task->book = this;
    task->seq = ++m_FetchSeq;
    task->chapter_index = index;
    task->pos = pos;
    task->url = (char*)malloc(url.size() + 1);
    if (!task->url)
    {
        free(task);
        return;
    }
    memcpy(task->url, url.c_str(), url.size() + 1);

    m_IsLoading = TRUE;
    m_FetchingIndex = index;
    PlayLoadingImage(hWnd);

    if (0 == _beginthreadex(NULL, 0, WebFetchThread, task, 0, &thread_id))
    {
        m_IsLoading = FALSE;
        free(task->url);
        free(task);
        StopLoadingImage(hWnd);
        MessageBox_(hWnd, IDS_REQUEST_ERROR, IDS_ERROR, MB_ICONERROR | MB_OK);
    }
}

void WebBook::JumpChapter(HWND hWnd, int index)
{
    if (index < 0 || index >= (int)m_Chapters.size())
        return;

    if (index == m_CurIndex && m_Text && m_Length > 0)
    {
        m_PendingPos = 0;
        ReDraw(hWnd);
        return;
    }

    RequestChapter(hWnd, index, 0);
}

void WebBook::JumpPrevChapter(HWND hWnd)
{
    if (m_CurIndex > 0)
        RequestChapter(hWnd, m_CurIndex - 1, -1);   // -1: 上一章末尾
}

void WebBook::JumpNextChapter(HWND hWnd)
{
    if (m_CurIndex + 1 < (int)m_Chapters.size())
        RequestChapter(hWnd, m_CurIndex + 1, 0);
}

BOOL WebBook::OnUpDownEvent(HWND hWnd, int draw_type)
{
    if (m_IsLoading)
        return FALSE;   // 正在取新章节，先吃掉这次翻页

    if (draw_type == DRAW_PAGE_UP || draw_type == DRAW_LINE_UP)
    {
        if (m_Index <= 0 && m_CurIndex > 0)
        {
            RequestChapter(hWnd, m_CurIndex - 1, -1);
            return FALSE;
        }
    }
    else if (draw_type == DRAW_PAGE_DOWN || draw_type == DRAW_LINE_DOWN)
    {
        if (m_Index + GetPageLength() >= m_Length && m_CurIndex + 1 < (int)m_Chapters.size())
        {
            RequestChapter(hWnd, m_CurIndex + 1, 0);
            return FALSE;
        }
    }

    return TRUE;
}

BOOL WebBook::OnDrawPageEvent(HWND hWnd)
{
    // 换章后第一次绘制前套用目标位置（必须发生在 CalcPageDown 之前）
    if (m_PendingPos >= 0 && m_Text && m_Length > 0)
    {
        int pos = m_PendingPos;
        if (pos > m_Length)
            pos = m_Length;
        if (pos < 0)
            pos = 0;
        m_Index = pos;
        m_PendingPos = -1;
    }

    m_PosInChapter = m_Index;
    SyncProgress(hWnd, FALSE);
    return TRUE;
}

LRESULT WebBook::OnBookEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (wParam == WEB_BE_CONTENT)
    {
        web_content_result_t* res = (web_content_result_t*)lParam;

        if (res && res->_this == (Book*)this && res->text && res->seq == m_FetchSeq
            && res->chapter_index == m_FetchingIndex)
        {
            wchar_t* text = res->text;
            int pos = res->pos;

            res->text = NULL;

            m_IsLoading = FALSE;
            m_FetchingIndex = -1;

            if (pos < 0)    // 上一章末尾
                pos = (m_PageLength > 0 && res->len > m_PageLength) ? (res->len - m_PageLength) : 0;

            ApplyChapter(hWnd, res->chapter_index, pos, text, res->len);
            if (_Book == this)
                StopLoadingImage(hWnd);
        }
        return 0;
    }

    if (wParam == WEB_BE_CONTENT_FAIL)
    {
        web_content_result_t* res = (web_content_result_t*)lParam;

        if (res && res->_this == (Book*)this && res->seq == m_FetchSeq)
        {
            m_IsLoading = FALSE;
            m_FetchingIndex = -1;
            if (_Book == this)
            {
                StopLoadingImage(hWnd);
                MessageBox_(hWnd, IDS_REQUEST_CONTENT_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
            }
        }
        return 0;
    }

    return 0;
}

// ---------------------------------------------------------------------------
// 进度同步
// ---------------------------------------------------------------------------

std::string WebBook::BuildProgressJson(void)
{
    cJSON* root = NULL;
    char* json = NULL;
    std::string ret;

    root = cJSON_CreateObject();
    if (!root)
        return ret;

    // 字段与手机网页版 saveBookProgress 完全一致（服务端按 name+author 找书）
    cJSON_AddStringToObject(root, "name", m_Meta.name.c_str());
    cJSON_AddStringToObject(root, "author", m_Meta.author.c_str());
    cJSON_AddNumberToObject(root, "durChapterIndex", (double)m_Meta.chapter_index);
    cJSON_AddNumberToObject(root, "durChapterPos", (double)m_Meta.chapter_pos);
    cJSON_AddNumberToObject(root, "durChapterTime", (double)NowMs());
    cJSON_AddStringToObject(root, "durChapterTitle", m_Meta.chapter_title.c_str());

    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json)
    {
        ret = json;
        free(json);
    }
    return ret;
}

void WebBook::PostProgress(BOOL wait_done)
{
    std::string body = BuildProgressJson();
    std::string url = BaseUrl("/saveBookProgress");
    web_post_task_t* task;
    unsigned thread_id;

    if (body.empty() || m_Meta.name.empty())
        return;

    if (wait_done)
    {
        char* resp = NULL;
        int len = 0;
        WebHttpRequest("POST", url.c_str(), body.c_str(), (int)body.size(), &resp, &len, WEB_POST_TIMEOUT_MS);
        if (resp)
            free(resp);
        return;
    }

    task = (web_post_task_t*)calloc(1, sizeof(web_post_task_t));
    if (!task)
        return;
    task->url = (char*)malloc(url.size() + 1);
    task->body = (char*)malloc(body.size() + 1);
    if (!task->url || !task->body)
    {
        free(task->url);
        free(task->body);
        free(task);
        return;
    }
    memcpy(task->url, url.c_str(), url.size() + 1);
    memcpy(task->body, body.c_str(), body.size() + 1);
    task->bodylen = (int)body.size();

    if (0 == _beginthreadex(NULL, 0, WebPostThread, task, 0, &thread_id))
    {
        free(task->url);
        free(task->body);
        free(task);
    }
}

void WebBook::FlushProgress(void)
{
    m_Meta.chapter_index = m_CurIndex;
    m_Meta.chapter_pos = m_PosInChapter;
    m_Meta.fetch_time = NowMs();
    m_Meta.chapter_time = NowMs();
    m_SavedPos = m_Meta.chapter_pos;
    m_SavedIndex = m_Meta.chapter_index;
    SaveMeta();
    m_LastSyncTick = GetTickCount();
    PostProgress(FALSE);
}

void WebBook::SyncProgress(HWND hWnd, BOOL force)
{
    DWORD now = GetTickCount();
    BOOL changed;

    m_Meta.chapter_index = m_CurIndex;
    m_Meta.chapter_pos = m_PosInChapter;
    m_Meta.fetch_time = NowMs();

    // 本地进度：位置一变就落盘（文件很小，翻页时写一次没负担），
    // 这样即使长时间不再翻页、或程序被强杀，阅读位置也不会丢
    changed = (m_Meta.chapter_pos != m_SavedPos || m_Meta.chapter_index != m_SavedIndex);
    if (changed || force)
    {
        m_SavedPos = m_Meta.chapter_pos;
        m_SavedIndex = m_Meta.chapter_index;
        m_Meta.chapter_time = NowMs();   // 推给手机后，手机端 durChapterTime 就是这个时刻
        SaveMeta();
    }

    // 手机端：5 秒节流，避免每翻一页都发请求
    if (!force && (now - m_LastSyncTick) < WEB_SYNC_INTERVAL_MS)
    {
        if (changed && 0 == s_SyncTimer)
        {
            s_SyncBook = this;
            s_SyncTimer = SetTimer(NULL, 0, WEB_SYNC_INTERVAL_MS, WebSyncTimerProc);
        }
        return;
    }

    if (s_SyncTimer)
    {
        KillTimer(NULL, s_SyncTimer);
        s_SyncTimer = 0;
    }
    s_SyncBook = NULL;
    m_LastSyncTick = now;
    PostProgress(FALSE);
}

// ---------------------------------------------------------------------------
// 从书架打开一本书：写 .web 文件后走正常打开流程
// ---------------------------------------------------------------------------

static unsigned int Fnv1a(const std::string& s)
{
    unsigned int h = 2166136261u;
    size_t i;

    for (i = 0; i < s.size(); i++)
    {
        h ^= (unsigned char)s[i];
        h *= 16777619u;
    }
    return h;
}

static void SanitizeFileName(const std::wstring& src, TCHAR* dst, int max)
{
    int i, n = 0;
    TCHAR c;

    for (i = 0; i < (int)src.size() && n < max - 1; i++)
    {
        c = src[i];
        if (c < 0x20 || _tcschr(_T("\\/:*?\"<>|"), c))
            c = _T('_');
        if (c == _T(' ') && n == 0)
            continue;
        dst[n++] = c;
    }
    dst[n] = 0;
}

void OnOpenWebBook(HWND hWnd, void* param)
{
    static TCHAR webdir[MAX_PATH] = { 0 };
    TCHAR path[MAX_PATH] = { 0 };
    TCHAR base[160] = { 0 };
    TCHAR name[128] = { 0 };
    web_book_meta_t* meta = (web_book_meta_t*)param;
    int i;

    if (!meta || meta->server.empty() || meta->book_url.empty())
        return;

    // <.exe 目录> 下的 .web 目录
    if (!webdir[0])
    {
        GetModuleFileName(NULL, webdir, sizeof(TCHAR) * (MAX_PATH - 1));
        for (i = (int)_tcslen(webdir) - 1; i >= 0; i--)
        {
            if (webdir[i] == _T('\\') || webdir[i] == _T('/'))
            {
                memcpy(&webdir[i + 1], WEB_FILE_SAVE_PATH, (_tcslen(WEB_FILE_SAVE_PATH) + 1) * sizeof(TCHAR));
                break;
            }
        }
        CreateDirectory(webdir, NULL);
        SetFileAttributes(webdir, FILE_ATTRIBUTE_HIDDEN);
    }

    SanitizeFileName(Utf8ToWide(meta->name.c_str()), name, 120);
    if (!name[0])
        _tcscpy(name, _T("book"));
    _stprintf(base, _T("%s-%08x"), name, Fnv1a(meta->book_url));

    _tcscpy(path, webdir);
    _tcscat(path, base);
    _tcscat(path, _T(".web"));

    meta->fetch_time = NowMs();
    WriteMetaFile(path, meta);

    OnOpenBook(hWnd, path, FALSE);
}

#endif // ENABLE_NETWORK
