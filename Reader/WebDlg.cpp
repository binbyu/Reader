#ifdef ENABLE_NETWORK

/*
 * WebDlg.cpp - "Web 阅读" 对话框
 *
 *   输入局域网里的 legado(开源阅读) web 服务地址(如 http://192.168.1.100:1122)
 *   -> 读取 /getBookshelf 书架（只显示网络书源的书，手机本地文件在 PC 上读不到）
 *   -> 选一本书 -> 用本软件的内核阅读，并与手机双向同步进度
 */

#include "WebDlg.h"
#include "WebBook.h"
#include "resource.h"
#include "types.h"
#include "Utils.h"
#include "cJSON.h"

#include <commctrl.h>
#include <shellapi.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <time.h>
#include <wctype.h>
#include <string>
#include <vector>
#include <algorithm>

extern HWND _hWnd;
extern HINSTANCE hInst;
extern int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType);
extern void OnOpenWebBook(HWND hWnd, void* param);

#define WM_WEB_SHELF_DONE       (WM_USER + 130)
#define WEB_SERVER_FILE         _T(".web\\server.txt")

// ---------------------------------------------------------------------------
// 书架数据
// ---------------------------------------------------------------------------

typedef struct web_shelf_item_t
{
    std::wstring name;
    std::wstring author;
    std::wstring dur_title;
    std::wstring latest;
    std::wstring source;        // 来源：网络书源的主机名，或“手机本地”
    std::string  book_url;      // 网络书的 http(s) 地址，或手机本地书的 content://、/storage/... 路径
    int          dur_index;
    int          dur_pos;
    int          total;
    long long    dur_time;
    BOOL         is_local;      // 手机本地书（origin=loc_*）
} web_shelf_item_t;

typedef struct web_shelf_result_t
{
    HWND hDlg;
    int  error;                                 // 0 成功，1 失败
    int  total;                                 // 服务端书架总数
    int  status;                                // http status
    int  bodylen;                               // 响应长度
    std::vector<web_shelf_item_t>* items;        // 对话框负责释放
} web_shelf_result_t;

typedef struct web_shelf_task_t
{
    HWND hDlg;
    char* url;
} web_shelf_task_t;

static std::vector<web_shelf_item_t> g_shelf;
static volatile LONG g_abandoned = 0;
static TCHAR g_server[512] = { 0 };
static int g_shelf_total = 0;

static INT_PTR CALLBACK WebDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// ---------------------------------------------------------------------------
// 小工具
// ---------------------------------------------------------------------------

static long long WebNowMs(void)
{
    FILETIME ft;
    long long t;

    GetSystemTimeAsFileTime(&ft);
    t = ((long long)ft.dwHighDateTime << 32) | (long long)ft.dwLowDateTime;
    return t / 10000 - 11644473600000LL;
}

static std::string WideToUtf8Str(const std::wstring& w)
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

static std::wstring Utf8ToWideStr(const char* s)
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

static int JsonInt(cJSON* obj, const char* key)
{
    cJSON* item = obj ? cJSON_GetObjectItem(obj, key) : NULL;

    if (item && cJSON_IsNumber(item))
        return (int)item->valuedouble;
    return 0;
}

static long long JsonLL(cJSON* obj, const char* key)
{
    cJSON* item = obj ? cJSON_GetObjectItem(obj, key) : NULL;

    if (item && cJSON_IsNumber(item))
        return (long long)item->valuedouble;
    return 0;
}

static void GetWebDir(TCHAR* dir, int size)
{
    int i;

    GetModuleFileName(NULL, dir, size - 1);
    for (i = (int)_tcslen(dir) - 1; i >= 0; i--)
    {
        if (dir[i] == _T('\\') || dir[i] == _T('/'))
        {
            _tcscpy(&dir[i + 1], _T(".web\\"));
            break;
        }
    }
}

static void GetServerFilePath(TCHAR* path, int size)
{
    TCHAR dir[MAX_PATH] = { 0 };

    GetWebDir(dir, MAX_PATH);
    CreateDirectory(dir, NULL);     // 已存在时返回失败，忽略即可
    SetFileAttributes(dir, FILE_ATTRIBUTE_HIDDEN);

    _tcsncpy(path, dir, size - 1);
    path[size - 1] = 0;
    _tcsncat(path, _T("server.txt"), size - _tcslen(path) - 1);
}

static void LoadServerUrl(TCHAR* buf, int size)
{
    TCHAR path[MAX_PATH] = { 0 };
    TCHAR* p = NULL;
    FILE* fp = NULL;

    buf[0] = 0;
    GetServerFilePath(path, MAX_PATH);
    fp = _tfopen(path, _T("rb"));
    if (!fp)
        return;
    if (fread(buf, sizeof(TCHAR), size - 1, fp) > 0)
        buf[size - 1] = 0;
    fclose(fp);

    // 去掉 BOM / 首尾空白（用记事本编辑过这个文件会带上 BOM）
    p = buf;
    while (*p == 0xFEFF || *p == _T(' ') || *p == _T('\t') || *p == _T('\r') || *p == _T('\n'))
        p++;
    if (p != buf)
        memmove(buf, p, (_tcslen(p) + 1) * sizeof(TCHAR));
    while (buf[0] && (buf[_tcslen(buf) - 1] == _T('\r') || buf[_tcslen(buf) - 1] == _T('\n')
                      || buf[_tcslen(buf) - 1] == _T(' ') || buf[_tcslen(buf) - 1] == _T('\t')))
        buf[_tcslen(buf) - 1] = 0;
}

static void SaveServerUrl(const TCHAR* url)
{
    TCHAR path[MAX_PATH] = { 0 };
    FILE* fp = NULL;

    GetServerFilePath(path, MAX_PATH);
    fp = _tfopen(path, _T("wb"));
    if (!fp)
        return;
    fwrite(url, sizeof(TCHAR), _tcslen(url), fp);
    fclose(fp);
    SetFileAttributes(path, FILE_ATTRIBUTE_HIDDEN);
}

// 允许只输入 192.168.1.2:1122
static void NormalizeServer(const TCHAR* src, TCHAR* dst, int size)
{
    int n = 0;

    // 顺手清掉粘贴/记事本带来的 BOM 和空白
    while (*src == 0xFEFF || *src == _T(' ') || *src == _T('\t') || *src == _T('\r') || *src == _T('\n'))
        src++;

    _tcsncpy(dst, src, size - 1);
    dst[size - 1] = 0;

    while (_tcslen(dst) > 0 && (dst[_tcslen(dst) - 1] == _T('/') || dst[_tcslen(dst) - 1] == _T(' ')
                                || dst[_tcslen(dst) - 1] == _T('\r') || dst[_tcslen(dst) - 1] == _T('\n')))
        dst[_tcslen(dst) - 1] = 0;

    if (_tcslen(dst) > 0 && NULL == _tcsstr(dst, _T("://")))
    {
        TCHAR tmp[512] = { 0 };
        _tcscpy(tmp, _T("http://"));
        _tcsncat(tmp, dst, 500);
        _tcscpy(dst, tmp);
    }
    (void)n;
}

static std::string UrlEncode(const std::string& s)
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

// ---------------------------------------------------------------------------
// 书架请求：后台线程用 WinHTTP 取，解析完 PostMessage 回对话框
// ---------------------------------------------------------------------------

static unsigned __stdcall WebShelfThread(void* param)
{
    web_shelf_task_t* task = (web_shelf_task_t*)param;
    web_shelf_result_t* res = NULL;
    cJSON* root = NULL;
    char* body = NULL;
    int len = 0;
    int status = 0;
    int i, n;

    if (!task)
        return 0;

    status = WebHttpRequest("GET", task->url, NULL, 0, &body, &len, 20000);

    if (1 == InterlockedCompareExchange(&g_abandoned, 1, 1))
    {
        // 对话框已经关了，自己收尾
        if (body)
            free(body);
        free(task->url);
        free(task);
        return 0;
    }

    res = new web_shelf_result_t;
    res->hDlg = task->hDlg;
    res->error = 0;
    res->total = 0;
    res->status = status;
    res->bodylen = len;
    res->items = new std::vector<web_shelf_item_t>;

    if (status == 200 && body)
        root = cJSON_Parse(body);

    if (root)
    {
        cJSON* data = cJSON_GetObjectItem(root, "data");
        n = cJSON_IsArray(data) ? cJSON_GetArraySize(data) : 0;

        for (i = 0; i < n; i++)
        {
            cJSON* book = cJSON_GetArrayItem(data, i);
            std::string origin, book_url;
            web_shelf_item_t item;

            if (!book)
                continue;

            origin = JsonStr(book, "origin");
            book_url = JsonStr(book, "bookUrl");
            res->total++;

            // 手机本地书(content:// 或 /storage/...)也能通过 web 服务读到正文，一并显示
            if (book_url.empty())
                continue;

            item.is_local = (origin.compare(0, 3, "loc") == 0) ? TRUE : FALSE;
            if (item.is_local)
            {
                item.source = _T("手机本地");
            }
            else
            {
                // origin 形如 https://fqbook.cc/ ，只留主机名
                std::string host = origin;
                size_t pos = host.find("://");
                if (pos != std::string::npos)
                    host = host.substr(pos + 3);
                pos = host.find_first_of("/#");
                if (pos != std::string::npos)
                    host = host.substr(0, pos);
                item.source = Utf8ToWideStr(host.c_str());
                if (item.source.empty())
                    item.source = _T("网络书源");
            }

            item.name = Utf8ToWideStr(JsonStr(book, "name").c_str());
            item.author = Utf8ToWideStr(JsonStr(book, "author").c_str());
            item.dur_title = Utf8ToWideStr(JsonStr(book, "durChapterTitle").c_str());
            item.latest = Utf8ToWideStr(JsonStr(book, "latestChapterTitle").c_str());
            item.book_url = book_url;
            item.dur_index = JsonInt(book, "durChapterIndex");
            item.dur_pos = JsonInt(book, "durChapterPos");
            item.total = JsonInt(book, "totalChapterNum");
            item.dur_time = JsonLL(book, "durChapterTime");

            if (!item.name.empty())
                res->items->push_back(item);
        }
        cJSON_Delete(root);
    }
    else
    {
        res->error = 1;
    }

    if (body)
        free(body);
    free(task->url);
    free(task);

    // 最近读的排前面
    std::sort(res->items->begin(), res->items->end(),
        [](const web_shelf_item_t& a, const web_shelf_item_t& b) { return a.dur_time > b.dur_time; });

    PostMessage(res->hDlg, WM_WEB_SHELF_DONE, 0, (LPARAM)res);
    return 0;
}

static void StartShelfRequest(HWND hDlg)
{
    TCHAR url[512] = { 0 };
    TCHAR server[512] = { 0 };
    char full[1024] = { 0 };
    char* ansi = NULL;
    web_shelf_task_t* task = NULL;
    unsigned thread_id;

    GetDlgItemText(hDlg, IDC_WEB_URL, url, 512);
    NormalizeServer(url, server, 512);
    if (!server[0])
    {
        SetDlgItemText(hDlg, IDC_WEB_STATUS, _T("请先填写 web 服务地址，例如 http://192.168.1.100:1122"));
        return;
    }

    _tcscpy(g_server, server);
    SaveServerUrl(g_server);
    g_abandoned = 0;

    ansi = Utf16ToUtf8(server);
    sprintf(full, "%s/getBookshelf", ansi);

    task = (web_shelf_task_t*)calloc(1, sizeof(web_shelf_task_t));
    if (!task)
        return;
    task->hDlg = hDlg;
    task->url = (char*)malloc(strlen(full) + 1);
    if (!task->url)
    {
        free(task);
        return;
    }
    strcpy(task->url, full);

    SetDlgItemText(hDlg, IDC_WEB_STATUS, _T("正在获取书架 ..."));

    if (0 == _beginthreadex(NULL, 0, WebShelfThread, task, 0, &thread_id))
    {
        free(task->url);
        free(task);
        SetDlgItemText(hDlg, IDC_WEB_STATUS, _T("无法创建网络线程"));
    }
}

// ---------------------------------------------------------------------------
// 列表
// ---------------------------------------------------------------------------

static void InitListColumns(HWND hList)
{
    struct { const TCHAR* name; int width; } cols[] =
    {
        { _T("书名"), 180 },
        { _T("作者"), 90 },
        { _T("进度"), 115 },
        { _T("最新章节"), 150 },
        { _T("最近阅读"), 110 },
        { _T("来源"), 100 },
    };
    LVCOLUMN lvc = { 0 };
    int i;

    for (i = 0; i < (int)(sizeof(cols) / sizeof(cols[0])); i++)
    {
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.iSubItem = i;
        lvc.pszText = (TCHAR*)cols[i].name;
        lvc.cx = cols[i].width;
        lvc.fmt = LVCFMT_LEFT;
        ListView_InsertColumn(hList, i, &lvc);
    }
}

// 关键词按空格分词，全部命中才算匹配；匹配 书名/作者/当前章节/最新章节
static BOOL MatchFilter(const web_shelf_item_t& item, const TCHAR* filter)
{
    std::wstring key(filter);
    std::wstring hay;
    size_t i, start = 0;

    if (key.empty())
        return TRUE;

    hay = item.name;
    hay += _T("\n");
    hay += item.author;
    hay += _T("\n");
    hay += item.dur_title;
    hay += _T("\n");
    hay += item.latest;

    for (i = 0; i < key.size(); i++)
        key[i] = (TCHAR)towlower(key[i]);
    for (i = 0; i < hay.size(); i++)
        hay[i] = (TCHAR)towlower(hay[i]);

    while (start <= key.size())
    {
        size_t sp = key.find(_T(' '), start);
        std::wstring word = (sp == std::wstring::npos) ? key.substr(start) : key.substr(start, sp - start);

        if (!word.empty() && hay.find(word) == std::wstring::npos)
            return FALSE;

        if (sp == std::wstring::npos)
            break;
        start = sp + 1;
    }
    return TRUE;
}

static void SetTextSafe(TCHAR* dst, int size, const TCHAR* src)
{
    if (!src)
        src = _T("");
    _tcsncpy(dst, src, size - 1);
    dst[size - 1] = 0;
}

static void FillList(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_WEB_LIST);
    TCHAR filter[128] = { 0 };
    TCHAR text[256] = { 0 };
    TCHAR status[512] = { 0 };
    int i, row = 0;
    int shown = 0;
    int local_count = 0;

    GetDlgItemText(hDlg, IDC_WEB_SEARCH, filter, 128);
    ListView_DeleteAllItems(hList);

    for (i = 0; i < (int)g_shelf.size(); i++)
    {
        const web_shelf_item_t& item = g_shelf[i];
        LVITEM lvi = { 0 };

        if (item.is_local)
            local_count++;

        if (!MatchFilter(item, filter))
            continue;

        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = row;
        lvi.lParam = (LPARAM)i;
        lvi.pszText = (TCHAR*)item.name.c_str();
        ListView_InsertItem(hList, &lvi);

        ListView_SetItemText(hList, row, 1, (TCHAR*)item.author.c_str());

        // 进度
        if (item.total > 0)
            _stprintf(text, _T("%d / %d 章"), item.dur_index + 1, item.total);
        else if (!item.dur_title.empty())
            SetTextSafe(text, 256, item.dur_title.c_str());
        else
            _tcscpy(text, _T("-"));
        ListView_SetItemText(hList, row, 2, text);

        // 最新章节（本地书没有这一项）
        if (item.latest.empty())
            _tcscpy(text, item.is_local ? _T("（手机本地文件）") : _T("-"));
        else
            SetTextSafe(text, 256, item.latest.c_str());
        ListView_SetItemText(hList, row, 3, text);

        // 最近阅读
        if (item.dur_time > 0)
        {
            time_t t = (time_t)(item.dur_time / 1000);
            struct tm* ptm = localtime(&t);
            if (ptm)
                _stprintf(text, _T("%04d-%02d-%02d %02d:%02d"), ptm->tm_year + 1900, ptm->tm_mon + 1,
                          ptm->tm_mday, ptm->tm_hour, ptm->tm_min);
            else
                _tcscpy(text, _T("-"));
        }
        else
        {
            _tcscpy(text, _T("-"));
        }
        ListView_SetItemText(hList, row, 4, text);

        // 来源
        ListView_SetItemText(hList, row, 5, (TCHAR*)(item.source.empty() ? _T("-") : item.source.c_str()));

        row++;
        shown++;
    }

    if (g_shelf.empty())
        _tcscpy(status, _T("没有可读的书籍（或连接失败）"));
    else if (filter[0])
        _stprintf(status, _T("“%s”命中 %d 本 / 可读 %d 本（回车打开第一本，点“清空”看全部）"),
                  filter, shown, (int)g_shelf.size());
    else
        _stprintf(status, _T("书架共 %d 本，可读 %d 本（手机本地 %d 本）：输入关键词即时筛选，双击书名阅读，进度与手机同步"),
                  g_shelf_total, (int)g_shelf.size(), local_count);

    SetDlgItemText(hDlg, IDC_WEB_STATUS, status);

    // 筛完自动选中第一行，直接回车就能打开，不用再点选
    if (row > 0)
    {
        ListView_SetItemState(hList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(hList, 0, FALSE);
    }
}

static BOOL OpenSelectedBook(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_WEB_LIST);
    int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    LVITEM lvi = { 0 };
    web_book_meta_t meta;
    int index;

    if (sel < 0)
    {
        SetDlgItemText(hDlg, IDC_WEB_STATUS, _T("请先在列表里选一本书"));
        return FALSE;
    }

    lvi.mask = LVIF_PARAM;
    lvi.iItem = sel;
    if (!ListView_GetItem(hList, &lvi))
        return FALSE;

    index = (int)lvi.lParam;
    if (index < 0 || index >= (int)g_shelf.size())
        return FALSE;

    meta.server = WideToUtf8Str(g_server);
    meta.book_url = g_shelf[index].book_url;
    meta.name = WideToUtf8Str(g_shelf[index].name);
    meta.author = WideToUtf8Str(g_shelf[index].author);
    meta.chapter_index = g_shelf[index].dur_index;
    meta.chapter_pos = g_shelf[index].dur_pos;
    meta.chapter_time = g_shelf[index].dur_time;
    meta.chapter_title = WideToUtf8Str(g_shelf[index].dur_title);
    meta.fetch_time = WebNowMs();       // 刚拿到的书架数据，开书时不必再拉一次

    OnOpenWebBook(_hWnd, &meta);
    return TRUE;
}

// ---------------------------------------------------------------------------
// 对话框
// ---------------------------------------------------------------------------

static INT_PTR CALLBACK WebDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_BOOK));
        HWND hList = GetDlgItem(hDlg, IDC_WEB_LIST);
        TCHAR url[512] = { 0 };

        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

        ListView_SetExtendedListViewStyleEx(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES,
                                            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        InitListColumns(hList);

        // 搜索框里的灰色提示（Common-Controls 6 的 cue banner）
        SendMessage(GetDlgItem(hDlg, IDC_WEB_SEARCH), 0x1501 /*EM_SETCUEBANNER*/, (WPARAM)TRUE,
                    (LPARAM)_T("输入书名/作者即时筛选，回车打开第一本"));

        g_shelf.clear();
        g_shelf_total = 0;
        g_abandoned = 0;

        LoadServerUrl(url, 512);
        if (url[0])
        {
            SetDlgItemText(hDlg, IDC_WEB_URL, url);
            _tcscpy(g_server, url);
            PostMessage(hDlg, WM_COMMAND, IDC_WEB_GO, 0);   // 自动连接上一次的地址
        }
        else
        {
            SetDlgItemText(hDlg, IDC_WEB_STATUS, _T("填写手机 legado 的 web 服务地址后点“连接”，例如 http://192.168.1.100:1122"));
        }
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (OpenSelectedBook(hDlg))
                EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;

        case IDCANCEL:
            g_abandoned = 1;    // 在途的书架请求会自己释放内存，不再回发消息
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;

        case IDC_WEB_GO:
            StartShelfRequest(hDlg);
            return (INT_PTR)TRUE;

        case IDC_WEB_SEARCH:    // 关键词输入框：边打字边筛选
            if (EN_CHANGE == HIWORD(wParam))
                FillList(hDlg);
            return (INT_PTR)TRUE;

        case IDC_WEB_REFRESH:   // 清空关键词，显示全部
            SetDlgItemText(hDlg, IDC_WEB_SEARCH, _T(""));
            FillList(hDlg);
            SetFocus(GetDlgItem(hDlg, IDC_WEB_SEARCH));
            return (INT_PTR)TRUE;

        case IDC_WEB_SYNC:
        {
            TCHAR cmd[1024] = { 0 };
            TCHAR url[512] = { 0 };

            GetDlgItemText(hDlg, IDC_WEB_URL, url, 512);
            NormalizeServer(url, g_server, 512);
            _stprintf(cmd, _T("%s/vue/index.html#/"), g_server);
            ShellExecute(hDlg, _T("open"), cmd, NULL, NULL, SW_SHOWNORMAL);
            return (INT_PTR)TRUE;
        }
        }
        break;

    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->idFrom == IDC_WEB_LIST && ((LPNMHDR)lParam)->code == NM_DBLCLK)
        {
            if (OpenSelectedBook(hDlg))
                EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        break;

    case WM_WEB_SHELF_DONE:
    {
        web_shelf_result_t* res = (web_shelf_result_t*)lParam;

        if (res)
        {
            if (res->error)
            {
                TCHAR msg[512] = { 0 };
                g_shelf.clear();
                ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_WEB_LIST));
                _stprintf(msg, _T("连接失败 [http=%d len=%d]：请确认地址正确、手机上的 web 服务已开启且在同一局域网"),
                          res->status, res->bodylen);
                SetDlgItemText(hDlg, IDC_WEB_STATUS, msg);
            }
            else
            {
                g_shelf = *res->items;
                g_shelf_total = res->total;
                FillList(hDlg);
                SetFocus(GetDlgItem(hDlg, IDC_WEB_SEARCH));   // 拿到书架后光标直接落在搜索框，可立即输入
            }
            delete res->items;
            delete res;
        }
        return (INT_PTR)TRUE;
    }

    case WM_SIZE:
    {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);

        if (w < 200 || h < 150)
            break;

        SetWindowPos(GetDlgItem(hDlg, IDC_WEB_URL), NULL, 7, 7, w - 112, 14, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hDlg, IDC_WEB_GO), NULL, w - 101, 7, 44, 14, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hDlg, IDC_WEB_SYNC), NULL, w - 53, 7, 46, 14, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hDlg, IDC_WEB_SEARCH), NULL, 7, 26, w - 112, 14, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hDlg, IDC_WEB_REFRESH), NULL, w - 101, 26, 94, 14, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hDlg, IDC_WEB_LIST), NULL, 7, 44, w - 14, h - 68, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hDlg, IDC_WEB_STATUS), NULL, 7, h - 18, w - 124, 12, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hDlg, IDOK), NULL, w - 111, h - 21, 50, 14, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hDlg, IDCANCEL), NULL, w - 57, h - 21, 50, 14, SWP_NOZORDER);
        break;
    }

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* info = (MINMAXINFO*)lParam;
        info->ptMinTrackSize.x = 480;
        info->ptMinTrackSize.y = 300;
        break;
    }
    }

    return (INT_PTR)FALSE;
}

void OpenWebDlg(void)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_WEB), _hWnd, WebDlgProc);
}

#endif // ENABLE_NETWORK
