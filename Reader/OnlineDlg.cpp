#ifdef ENABLE_NETWORK
#include "OnlineDlg.h"
#include "BooksourceDlg.h"
#include "resource.h"
#include "HtmlParser.h"
#include "https.h"
#include "Utils.h"

extern header_t* _header;
extern HWND _hWnd;
extern HINSTANCE hInst;
extern void OnOpenOlBook(HWND, void*);
extern int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType);
extern int MessageBoxFmt_(HWND hWnd, UINT captionId, UINT uType, UINT formatId, ...);
extern void combine_url(const char* path, const char* url, char* dsturl);

typedef struct req_query_param_t {
    HWND hDlg;
    TCHAR text[256];
    int bs_idx;
    int is_global;
} req_query_param_t;

static BOOL g_Enable = TRUE;
static req_handler_t g_hRequest = NULL;
static int g_lastPos = 0;
static req_query_param_t* g_query_param = NULL;

static INT_PTR CALLBACK OnlineDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static int OnRequestQuery(http_charset_t charset);
static int OnRequestCharset(void);
static void EnableDialog(HWND hDlg, BOOL enable);
static BOOL _begin_query(HWND hDlg);
static void _end_query(HWND hDlg);

void OpenOnlineDlg(void)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_ONLINE), _hWnd, OnlineDlgProc);
}

void DumpParseErrorFile(const char* html, int htmllen)
{
#if TEST_MODEL
    const char* UTF_8_BOM = "\xEF\xBB\xBF";
    FILE* fp;
    if (html && htmllen > 0)
    {
        fp = fopen("dump.html", "wb");
        if (fp)
        {
            fwrite(UTF_8_BOM, 1, 3, fp);
            fwrite(html, 1, htmllen, fp);
            fclose(fp);
        }
    }
#endif
}

#if TEST_MODEL
void TestXpathFromDump(void)
{
    FILE* fp;
    char* html;
    int htmllen;
    void* doc = NULL;
    void* ctx = NULL;
    BOOL fkill = FALSE;
    const char* xpath1 = "//div[@id='list']/dl/dd[position()>12]/a";
    const char* xpath2 = "//div[@id='list']/dl/dd[position()>12]/a/@href";
    std::vector<std::string> value1, value2;

    fp = fopen("dump.html", "rb");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        htmllen = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        html = (char*)malloc(htmllen + 1);
        fread(html, 1, htmllen, fp);
        fclose(fp);

        // do check
        HtmlParser::Instance()->HtmlParseBegin(html, htmllen, &doc, &ctx, &fkill);
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, xpath1, value1, &fkill);
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, xpath2, value2, &fkill);
        HtmlParser::Instance()->HtmlParseEnd(doc, ctx);

        free(html);
    }
}
#endif

void ReloadBookSourceCombobox(HWND hDlg, int iPos)
{
    int i;
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_RESETCONTENT, 0, NULL);
    for (i = 0; i < _header->book_source_count; i++)
    {
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_ADDSTRING, 0, (LPARAM)_header->book_sources[i].title);
    }
#if ENABLE_GLOBAL_SEARCH
    if (_header->book_source_count > 0)
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_ADDSTRING, 0, (LPARAM)_T("ALL"));
    if (iPos < 0 || iPos > _header->book_source_count)
#else
    if (g_lastPos < 0 || g_lastPos >= _header->book_source_count)
#endif
        iPos = 0;
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_SETCURSEL, iPos, NULL);
    g_lastPos = iPos;
}

static INT_PTR CALLBACK OnlineDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR path[1024] = {0};
    HWND hList;
    int iPos;
    int i, colnum;
    int idx;
    ol_book_param_t param = {0};
    HWND hHeader = NULL;
    LVITEM lvi;

    switch (message)
    {
    case WM_INITDIALOG:
    {
        g_hRequest = NULL;
        g_Enable = TRUE;
        HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_BOOK));
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        ListView_SetExtendedListViewStyleEx(GetDlgItem(hDlg, IDC_LIST_QUERY), LVS_REPORT | LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
        ReloadBookSourceCombobox(hDlg, g_lastPos);
        _end_query(hDlg);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            hList = GetDlgItem(hDlg, IDC_LIST_QUERY);
            memset(&lvi, 0, sizeof(LVITEM));
            lvi.mask = LVIF_PARAM;
            lvi.iItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            lvi.iSubItem = 0;
            if (ListView_GetItem(hList, &lvi) == TRUE)
            {
                idx = (int)lvi.lParam; // get book source index
                iPos = lvi.iItem;

                colnum = 2;
                colnum++; // author
                ListView_GetItemText(hList, iPos, colnum, path, 1024);
                ListView_GetItemText(hList, iPos, 1, param.book_name, 256);
                strcpy(param.main_page, Utf16ToUtf8(path));
                strcpy(param.host, _header->book_sources[idx].host);
                OnOpenOlBook(_hWnd, &param);
                g_lastPos = idx;
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
        case IDCANCEL:
            if (g_hRequest)
            {
                hapi_cancel(g_hRequest);
            }
            g_hRequest = NULL;
            g_Enable = TRUE;
            g_lastPos = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_BUTTON_BS_MANAGER:
            BS_OpenDlg(hDlg);
            break;
        case IDC_BUTTON_OL_QUERY:
            if (g_Enable)
            {
                if (_begin_query(hDlg))
                {
                    hList = GetDlgItem(hDlg, IDC_LIST_QUERY);
                    ListView_DeleteAllItems(hList);
                    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
                    colnum = (int)SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0);
                    for (i = colnum - 1; i >= 0; i--)
                        SendMessage(hList, LVM_DELETECOLUMN, i, 0);
                    OnRequestCharset();
                }
            }
            else
            {
                if (g_hRequest)
                {
                    hapi_cancel(g_hRequest);
                }
            }
            break;
        default:
            break;
        }
        break;

    case WM_SIZE:
    {
        const int client_width = LOWORD(lParam);
        const int client_height = HIWORD(lParam);

        const HWND hListView = GetDlgItem(hDlg, IDC_LIST_QUERY);
        SetWindowPos(hListView, nullptr, 11, 42, client_width - 24, client_height - 88, SWP_NOZORDER);

        const HWND hBsManagerBtn = GetDlgItem(hDlg, IDC_BUTTON_BS_MANAGER);
        SetWindowPos(hBsManagerBtn, nullptr, 11, client_height - 34, 75, 23, SWP_NOZORDER);

        const HWND hOkBtn = GetDlgItem(hDlg, IDOK);
        SetWindowPos(hOkBtn, nullptr, client_width - 166, client_height - 34, 75, 23, SWP_NOZORDER);

        const HWND hCancelBtn = GetDlgItem(hDlg, IDCANCEL);
        SetWindowPos(hCancelBtn, nullptr, client_width - 86, client_height - 34, 75, 23, SWP_NOZORDER);
        break;
    }
    case WM_GETMINMAXINFO: // 设置弹框最小尺寸
    {
        MINMAXINFO* pMinMaxInfo = (MINMAXINFO*)lParam;
        // 指定最小宽度和最小高度
        pMinMaxInfo->ptMinTrackSize.x = 480; // 最小宽度
        pMinMaxInfo->ptMinTrackSize.y = 340; // 最小高度
    }
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

static unsigned int RequestQueryCompleter(request_result_t* result)
{
    char* html = NULL;
    int htmllen = 0;
    HWND hDlg = g_query_param->hDlg;
    int bs_idx = g_query_param->bs_idx;
    std::vector<std::string> table_name;
    std::vector<std::string> table_url;
    std::vector<std::string> table_author;
    HWND hList = NULL;
    LV_COLUMN lvc = {0};
    LVITEM lvitem = {0};
    int i, col;
    void* doc = NULL;
    void* ctx = NULL;
    BOOL cancel = FALSE;
    TCHAR colname[256] = {0};
    char Url[1024] = {0};
    int needfree = 0;
    HWND hHeader = NULL;
    int colnum = 0;

    g_hRequest = NULL;

    if (result->cancel)
    {
        _end_query(hDlg);
        return 1;
    }

    if (result->errno_ != succ)
    {
        if (g_query_param->is_global)
            goto _next;
        _end_query(hDlg);
        MessageBox_(hDlg, IDS_NETWORK_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    if (bs_idx < 0 || bs_idx >= _header->book_source_count)
    {
        if (g_query_param->is_global)
            goto _next;
        _end_query(hDlg);
        MessageBox_(hDlg, IDS_SELECT_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    if (result->status_code != 200)
    {
        if (g_query_param->is_global)
            goto _next;
        _end_query(hDlg);
        MessageBoxFmt_(hDlg, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_REQUEST_ERROR, result->status_code);
        return 1;
    }

    html = result->body;
    htmllen = result->bodylen;

#if 0
    if (hapi_get_charset(result->header) != utf_8)
#else
    if (!is_utf8(html, htmllen)) // fixed bug, focus check encode
#endif
    {
        wchar_t* tempbuf = NULL;
        int templen = 0;
        char* utf8buf = NULL;
        int utf8len = 0;
        // convert 'gbk' to 'utf-8'
        tempbuf = ansi_to_utf16(html, htmllen, &templen);
        utf8buf = utf16_to_utf8(tempbuf, templen, &utf8len);
        free(tempbuf);
        if (needfree)
        {
            free(html);
        }
        html = utf8buf;
        htmllen = utf8len;
        needfree = 1;
    }

    HtmlParser::Instance()->HtmlParseBegin(html, htmllen, &doc, &ctx, &cancel);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[bs_idx].book_name_xpath, table_name, &cancel, TRUE);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[bs_idx].book_mainpage_xpath, table_url, &cancel);
    if (_header->book_sources[bs_idx].book_author_xpath[0])
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[bs_idx].book_author_xpath, table_author, &cancel, TRUE);
    HtmlParser::Instance()->HtmlParseEnd(doc, ctx);

    // check value
    if (table_url.empty() || table_name.size() != table_url.size())
    {
        if (g_query_param->is_global)
            goto _next;

        DumpParseErrorFile(html, htmllen);

        if (needfree)
            free(html);
        _end_query(hDlg);
        MessageBox_(hDlg, IDS_PARSE_FAIL, IDS_WARN, MB_ICONWARNING | MB_OK);
        return 1;
    }

    if (needfree)
        free(html);

    hList = GetDlgItem(hDlg, IDC_LIST_QUERY);
    if (hList)
    {
        hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
        colnum = (int)SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0);

        // add list header
        if (colnum == 0)
        {
            col = 0;
            // book source name
            LoadString(hInst, IDS_BOOK_SOURCE, colname, 256);
            memset(&lvc, 0, sizeof(LV_COLUMN));
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            lvc.pszText = colname;
            lvc.cx = 80;
            SendMessage(hList, LVM_INSERTCOLUMN, col++, (LPARAM)&lvc);

            // book name
            LoadString(hInst, IDS_BOOK_NAME, colname, 256);
            memset(&lvc, 0, sizeof(LV_COLUMN));
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            lvc.pszText = colname;
            lvc.cx = 120;
            SendMessage(hList, LVM_INSERTCOLUMN, col++, (LPARAM)&lvc);

            // book author
            //if (g_query_param->is_global || !table_author.empty())
            {
                LoadString(hInst, IDS_AUTHOR, colname, 256);
                memset(&lvc, 0, sizeof(LV_COLUMN));
                lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
                lvc.pszText = colname;
                lvc.cx = 100;
                SendMessage(hList, LVM_INSERTCOLUMN, col++, (LPARAM)&lvc);
            }

            // mainpage
            LoadString(hInst, IDS_MAINPAGE, colname, 256);
            memset(&lvc, 0, sizeof(LV_COLUMN));
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            lvc.pszText = colname;
            lvc.cx = 180;
            SendMessage(hList, LVM_INSERTCOLUMN, col++, (LPARAM)&lvc);
        }

        colnum = ListView_GetItemCount(hList); // rownum
        for (i = 0; i < (int)table_name.size(); i++)
        {
            col = 0;
            // book source name
            lvitem.mask = LVIF_TEXT | LVIF_PARAM;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = i + colnum;
            lvitem.iSubItem = col++;
            lvitem.pszText = _header->book_sources[bs_idx].title;
            lvitem.lParam = bs_idx;
            ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);

            // book name
            memset(&lvitem, 0, sizeof(LVITEM));
            lvitem.mask = LVIF_TEXT;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = i + colnum;
            lvitem.iSubItem = col++;
            lvitem.pszText = Utf8ToUtf16(table_name[i].c_str());
            ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);

            // book author
            if (!table_author.empty() && i < (int)table_author.size())
            {
                memset(&lvitem, 0, sizeof(LVITEM));
                lvitem.mask = LVIF_TEXT;
                lvitem.cchTextMax = MAX_PATH;
                lvitem.iItem = i + colnum;
                lvitem.iSubItem = col;
                lvitem.pszText = Utf8ToUtf16(table_author[i].c_str());
                ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
                ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);
            }
            col++;

            // mainpage
            combine_url(table_url[i].c_str(), result->req->url, Url);
            memset(&lvitem, 0, sizeof(LVITEM));
            lvitem.mask = LVIF_TEXT;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = i + colnum;
            lvitem.iSubItem = col++;
            lvitem.pszText = Utf8ToUtf16(Url);
            ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);
        }
    }

_next:
    if (g_query_param->is_global && (++bs_idx < _header->book_source_count))
    {
        g_query_param->bs_idx++;
        OnRequestCharset();
        return 0;
    }

    _end_query(hDlg);
    return 0;
}

static int OnRequestQuery(http_charset_t charset)
{
    char* query_format;
    char url[1024];
    char content[1024] = {0};
    char* encode;
    request_t req;
    char* keyword = NULL;
    int bs_idx = g_query_param->bs_idx;

    if (charset == utf_8)
        keyword = Utf16ToUtf8(g_query_param->text);
    else
        keyword = Utf16ToAnsi(g_query_param->text);

    if (_header->book_sources[bs_idx].query_method == 0) // GET
    {
        query_format = _header->book_sources[bs_idx].query_url;

        hapi_url_encode(keyword, &encode);
        sprintf(url, query_format, encode);
        hapi_buffer_free(encode);
    }
    else // POST
    {
        strcpy(url, _header->book_sources[bs_idx].query_url);

        hapi_url_encode(keyword, &encode);
        sprintf(content, _header->book_sources[bs_idx].query_params, encode);
        hapi_buffer_free(encode);
    }

    // do request
    memset(&req, 0, sizeof(request_t));
    req.method = _header->book_sources[bs_idx].query_method == 0 ? GET : POST;
    req.url = url;
    req.content = content;
    req.content_length = (int)strlen(content);
    req.completer = RequestQueryCompleter;
    req.param1 = NULL;
    req.param2 = NULL;

    g_hRequest = hapi_request(&req);
    return 0;
}

static unsigned int RequestCharsetCompleter(request_result_t* result)
{
    HWND hDlg = g_query_param->hDlg;
    int bs_idx = g_query_param->bs_idx;

    g_hRequest = NULL;

    if (result->cancel)
    {
        _end_query(hDlg);
        return 1;
    }

    if (result->errno_ != succ)
    {
        if (g_query_param->is_global)
            goto _next;
        _end_query(hDlg);
        MessageBox_(hDlg, IDS_NETWORK_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    if (result->status_code != 200)
    {
        if (g_query_param->is_global)
            goto _next;
        _end_query(hDlg);
        MessageBoxFmt_(hDlg, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_REQUEST_ERROR, result->status_code);
        return 1;
    }

    OnRequestQuery(hapi_get_charset(result->header));
    return 0;

_next:
    if (g_query_param->is_global && (++bs_idx < _header->book_source_count))
    {
        g_query_param->bs_idx++;
        OnRequestCharset();
        return 0;
    }
    _end_query(hDlg);
    return 0;
}

static int OnRequestCharset(void)
{
    request_t req;
    char* query_format;
    char url[1024];
    char content[1024] = {0};
    char* encode;
    char* keyword = NULL;
    http_charset_t charset;
    int bs_idx = g_query_param->bs_idx;

    if (_header->book_sources[bs_idx].query_charset != 0) // 0: auto
    {
        if (_header->book_sources[bs_idx].query_charset == 1) // utf8
            charset = utf_8;
        else
            charset = gbk;
        return OnRequestQuery(charset);
    }
    keyword = Utf16ToUtf8(g_query_param->text);
    if (_header->book_sources[bs_idx].query_method == 0) // GET
    {
        query_format = _header->book_sources[bs_idx].query_url;

        hapi_url_encode(keyword, &encode);
        sprintf(url, query_format, encode);
        hapi_buffer_free(encode);
    }
    else // POST
    {
        strcpy(url, _header->book_sources[bs_idx].query_url);

        hapi_url_encode(keyword, &encode);
        sprintf(content, _header->book_sources[bs_idx].query_params, encode);
        hapi_buffer_free(encode);
    }

    // do request    
    memset(&req, 0, sizeof(request_t));
    req.method = HEAD;
    req.url = url;
#if 0
    req.content = content;
    req.content_length = strlen(content);
#endif
    req.completer = RequestCharsetCompleter;
    req.param1 = NULL;
    req.param2 = NULL;

    g_hRequest = hapi_request(&req);
    return 0;
}

static void EnableDialog(HWND hDlg, BOOL enable)
{
    TCHAR szQuery[256] = {0};
    TCHAR szStopQuery[256] = {0};
    LoadString(hInst, IDS_QUERY, szQuery, 256);
    LoadString(hInst, IDS_STOP_QUERY, szStopQuery, 256);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_QUERY_KEYWORD), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BS_MANAGER), enable);
    EnableWindow(GetDlgItem(hDlg, IDOK), enable);
    //EnableWindow(GetDlgItem(hDlg, IDCANCEL), enable);
    g_Enable = enable;
    if (enable)
        SetDlgItemText(hDlg, IDC_BUTTON_OL_QUERY, szQuery);
    else
        SetDlgItemText(hDlg, IDC_BUTTON_OL_QUERY, szStopQuery);
}

static BOOL _begin_query(HWND hDlg)
{
    if (_header->book_source_count == 0)
    {
        if (IDYES == MessageBox_(hDlg, IDS_NOTEXIST_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_YESNO))
        {
            BS_OpenDlg(hDlg);
        }
        goto _failed;
    }

    if (!g_query_param)
        g_query_param = (req_query_param_t*)malloc(sizeof(req_query_param_t));
    if (!g_query_param)
        goto _failed;
    memset(g_query_param, 0, sizeof(req_query_param_t));
    g_query_param->hDlg = hDlg;

    GetDlgItemText(hDlg, IDC_EDIT_QUERY_KEYWORD, g_query_param->text, 256);
    if (_tcslen(g_query_param->text) == 0)
    {
        MessageBox_(hDlg, IDS_EMPTY_KEYWORD, IDS_ERROR, MB_ICONERROR | MB_OK);
        goto _failed;
    }

    g_query_param->bs_idx = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
#if ENABLE_GLOBAL_SEARCH
    if (g_query_param->bs_idx < 0 || g_query_param->bs_idx > _header->book_source_count)
#else
    if (g_query_param->bs_idx < 0 || g_query_param->bs_idx >= _header->book_source_count)
#endif    
    {
        MessageBox_(hDlg, IDS_SELECT_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_OK);
        goto _failed;
    }

    if (g_query_param->bs_idx == _header->book_source_count)
    {
        g_query_param->bs_idx = 0;
        g_query_param->is_global = 1;
    }

    EnableDialog(hDlg, FALSE);
    return TRUE;

_failed:
    _end_query(hDlg);
    return FALSE;
}

static void _end_query(HWND hDlg)
{
    if (g_query_param)
    {
        free(g_query_param);
        g_query_param = NULL;
    }
    EnableDialog(hDlg, TRUE);
}

#endif
