#include "stdafx.h"
#ifdef ENABLE_NETWORK
#include "OnlineDlg.h"
#include "BooksourceDlg.h"
#include "resource.h"
#include "HtmlParser.h"
#include "httpclient.h"
#include "Utils.h"

extern header_t *_header;
extern HWND _hWnd;
extern HINSTANCE hInst;
extern void OnOpenOlBook(HWND, void*);
extern int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType);
extern int MessageBoxFmt_(HWND hWnd, UINT captionId, UINT uType, UINT formatId, ...);
extern BOOL Redirect(request_t *r, const char *url, req_handler_t *hReq);
extern void CombineUrl(const char *path, const char *url, char *dsturl);

static BOOL g_Enable = TRUE;
static req_handler_t g_hRequest = NULL;
static int g_lastPos = 0;

static INT_PTR CALLBACK OnlineDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static int OnRequestQuery(http_charset_t charset, HWND hDlg);
static int OnRequestCharset(HWND hDlg);
static void EnableDialog(HWND hDlg, BOOL enable);

void OpenOnlineDlg(void)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_ONLINE), _hWnd, OnlineDlgProc);
}

void DumpParseErrorFile(const char *html, int htmllen)
{
#if TEST_MODEL
    const char* UTF_8_BOM = "\xEF\xBB\xBF";
    FILE *fp;
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
    FILE *fp;
    char *html;
    int htmllen;
    void *doc = NULL;
    void *ctx = NULL;
    bool fkill = false;
    const char *xpath1 = "//div[@id='list']/dl/dd[position()>12]/a";
    const char *xpath2 = "//div[@id='list']/dl/dd[position()>12]/a/@href";
    std::vector<std::string> value1, value2;

    fp = fopen("dump.html", "rb");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        htmllen = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        html = (char *)malloc(htmllen+1);
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

static INT_PTR CALLBACK OnlineDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char* keyword = NULL;
    TCHAR text[256] = { 0 };
    TCHAR path[1024] = { 0 };
    HWND hList;
    int iPos;
    int i,colnum;
    int idx;
    ol_book_param_t param = { 0 };
    HWND hHeader = NULL;
    LVITEM lvi;

    switch (message)
    {
    case WM_INITDIALOG:
        g_hRequest = NULL;
        g_Enable = TRUE;
        ListView_SetExtendedListViewStyleEx(GetDlgItem(hDlg, IDC_LIST_QUERY), LVS_REPORT|LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_AUTOSIZECOLUMNS|LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
        for (i = 0; i < _header->book_source_count; i++)
        {
            SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_ADDSTRING, 0, (LPARAM)_header->book_sources[i].title);
        }
        if (g_lastPos < 0 || g_lastPos >= _header->book_source_count)
            g_lastPos = 0;
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_SETCURSEL, g_lastPos, NULL);
        EnableDialog(hDlg, TRUE);
        return (INT_PTR)TRUE;
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
                if (_header->book_sources[idx].book_author_xpath[0])
                    colnum++;
                if (_header->book_sources[idx].book_status_pos == 1)
                {
                    ListView_GetItemText(hList, iPos, colnum, text, 256);
                    if (_tcscmp(text, Utils::Utf8ToUtf16(_header->book_sources[idx].book_status_keyword)) == 0)
                        param.is_finished = 1;
                    colnum++;
                }
                ListView_GetItemText(hList, iPos, colnum, path, 1024);
                ListView_GetItemText(hList, iPos, 1, param.book_name, 256);
                strcpy(param.main_page, Utils::Utf16ToUtf8(path));
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
            g_lastPos = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_BUTTON_BS_MANAGER:
            BS_OpenDlg(hDlg);
            break;
        case IDC_BUTTON_OL_QUERY:
            if (g_Enable)
            {
                if (_header->book_source_count == 0)
                {
                    if (IDYES == MessageBox_(hDlg, IDS_NOTEXIST_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_YESNO))
                    {
                        BS_OpenDlg(hDlg);
                    }
                    break;
                }

                EnableDialog(hDlg, FALSE);
                hList = GetDlgItem(hDlg, IDC_LIST_QUERY);
                ListView_DeleteAllItems(hList);
                hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
                colnum = SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0);
                for (i=colnum-1; i>=0; i--)
                    SendMessage(hList, LVM_DELETECOLUMN, i, 0);
                OnRequestCharset(hDlg);
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
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

static unsigned int RequestQueryCompleter(request_result_t *result)
{
    char* html = NULL;
    int htmllen = 0;
    HWND hDlg = (HWND)result->param1;
    UINT bs_idx = ((UINT)result->param2) & 0xFFFF;
    UINT bs_start = ((UINT)result->param2) >> 16;
    std::vector<std::string> table_name;
    std::vector<std::string> table_url;
    std::vector<std::string> table_author;
    std::vector<std::string> table_status;
    HWND hList = NULL;
    LV_COLUMN lvc = { 0 };
    LVITEM lvitem = { 0 };
    int i,col;
    void* doc = NULL;
    void* ctx = NULL;
    bool cancel = false;
    TCHAR colname[256] = { 0 };
    char Url[1024] = { 0 };
    int needfree = 0;
    HWND hHeader = NULL;
    int colnum = 0;

    g_hRequest = NULL;

    if (result->cancel)
    {
        EnableDialog(hDlg, TRUE);
        return 1;
    }

    if (result->errno_ != succ)
    {
        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_NETWORK_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    if (bs_idx < 0 || bs_idx >= (UINT)_header->book_source_count)
    {
        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_SELECT_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    if (result->status_code != 200)
    {
        // redirect
        if (Redirect(result->req, hapi_get_location(result->header), &g_hRequest))
        {
            return 0;
        }
        EnableDialog(hDlg, TRUE);
        MessageBoxFmt_(hDlg, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_REQUEST_ERROR, result->status_code);
        return 1;
    }

    if (hapi_is_gzip(result->header))
    {
        needfree = 1;
        if (!Utils::gzipInflate((unsigned char*)result->body, result->bodylen, (unsigned char**)&html, &htmllen))
        {
            EnableDialog(hDlg, TRUE);
            MessageBox_(hDlg, IDS_DECOMPRESS_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
            return 1;
        }
    }
    else
    {
        html = result->body;
        htmllen = result->bodylen;
    }

#if 0
    if (hapi_get_charset(result->header) != utf_8)
#else
    if (!Utils::is_utf8(html, htmllen)) // fixed bug, focus check encode
#endif
    {
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

    HtmlParser::Instance()->HtmlParseBegin(html, htmllen, &doc, &ctx, &cancel);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[bs_idx].book_name_xpath, table_name, &cancel, true);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[bs_idx].book_mainpage_xpath, table_url, &cancel);
    if (_header->book_sources[bs_idx].book_author_xpath[0])
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[bs_idx].book_author_xpath, table_author, &cancel, true);
    if (_header->book_sources[bs_idx].book_status_pos == 1)
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[bs_idx].book_status_xpath, table_status, &cancel, true);
    HtmlParser::Instance()->HtmlParseEnd(doc, ctx);

    // check value
    if (table_url.empty() || table_name.size() != table_url.size())
    {
        DumpParseErrorFile(html, htmllen);

        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_PARSE_FAIL, IDS_WARN, MB_ICONWARNING | MB_OK);
        return 1;
    }

    if (needfree)
        free(html);

    hList = GetDlgItem(hDlg, IDC_LIST_QUERY);
    if (hList)
    {
        hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
        colnum = SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0);

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
            if (!table_author.empty())
            {
                LoadString(hInst, IDS_AUTHOR, colname, 256);
                memset(&lvc, 0, sizeof(LV_COLUMN));
                lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
                lvc.pszText = colname;
                lvc.cx = 60;
                SendMessage(hList, LVM_INSERTCOLUMN, col++, (LPARAM)&lvc);
            }

            // book status
            if (!table_status.empty())
            {
                LoadString(hInst, IDS_STATUS, colname, 256);
                memset(&lvc, 0, sizeof(LV_COLUMN));
                lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
                lvc.pszText = colname;
                lvc.cx = 60;
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
        
        for (i = 0; i < (int)table_name.size(); i++)
        {
            col = 0;
            // book source name
            lvitem.mask = LVIF_TEXT | LVIF_PARAM;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = i;
            lvitem.iSubItem = col++;
            lvitem.pszText = _header->book_sources[bs_idx].title;
            lvitem.lParam = bs_idx;
            ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);

            // book name
            memset(&lvitem, 0, sizeof(LVITEM));
            lvitem.mask = LVIF_TEXT;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = i;
            lvitem.iSubItem = col++;
            lvitem.pszText = Utils::Utf8ToUtf16(table_name[i].c_str());
            ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);

            // book author
            if (!table_author.empty() && i < (int)table_author.size())
            {
                memset(&lvitem, 0, sizeof(LVITEM));
                lvitem.mask = LVIF_TEXT;
                lvitem.cchTextMax = MAX_PATH;
                lvitem.iItem = i;
                lvitem.iSubItem = col++;
                lvitem.pszText = Utils::Utf8ToUtf16(table_author[i].c_str());
                ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
                ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);
            }

            // book status
            if (!table_status.empty() && i < (int)table_status.size())
            {
                memset(&lvitem, 0, sizeof(LVITEM));
                lvitem.mask = LVIF_TEXT;
                lvitem.cchTextMax = MAX_PATH;
                lvitem.iItem = i;
                lvitem.iSubItem = col++;
                lvitem.pszText = Utils::Utf8ToUtf16(table_status[i].c_str());
                ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
                ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);
            }

            // mainpage
            CombineUrl(table_url[i].c_str(), result->req->url, Url);
            memset(&lvitem, 0, sizeof(LVITEM));
            lvitem.mask = LVIF_TEXT;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = i;
            lvitem.iSubItem = col++;
            lvitem.pszText = Utils::Utf8ToUtf16(Url);
            ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);
        }
    }

    EnableDialog(hDlg, TRUE);
    return 0;
}

static int OnRequestQuery(http_charset_t charset, HWND hDlg)
{
    char* query_format;
    char url[1024];
    char content[1024] = { 0 };
    char* encode;
    request_t req;
    int sel;
    char *keyword = NULL;
    TCHAR text[256] = {0};

    GetDlgItemText(hDlg, IDC_EDIT_QUERY_KEYWORD, text, 256);
    if (_tcslen(text) == 0)
    {
        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_EMPTY_KEYWORD, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    sel = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
    if (sel < 0 || sel >= _header->book_source_count)
    {
        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_SELECT_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    if (charset == utf_8)
        keyword = Utils::Utf16ToUtf8(text);
    else
        keyword = Utils::Utf16ToAnsi(text);

    if (_header->book_sources[sel].query_method == 0) // GET
    {
        query_format = _header->book_sources[sel].query_url;

        Utils::UrlEncode(keyword, &encode);
        sprintf(url, query_format, encode);
        Utils::UrlFree(encode);
    }
    else // POST
    {
        strcpy(url, _header->book_sources[sel].query_url);

        Utils::UrlEncode(keyword, &encode);
        sprintf(content, _header->book_sources[sel].query_params, encode);
        Utils::UrlFree(encode);
    }

    // do request    
    memset(&req, 0, sizeof(request_t));
    req.method = _header->book_sources[sel].query_method == 0 ? GET : POST;
    req.url = url;
    req.content = content;
    req.content_length = strlen(content);
    req.completer = RequestQueryCompleter;
    req.param1 = hDlg;
    req.param2 = (void*)sel;

    g_hRequest = hapi_request(&req);
    return 0;
}

static unsigned int RequestCharsetCompleter(request_result_t *result)
{
    HWND hDlg = (HWND)result->param1;

    g_hRequest = NULL;

    if (result->cancel)
    {
        EnableDialog(hDlg, TRUE);
        return 1;
    }

    if (result->errno_ != succ)
    {
        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_NETWORK_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    if (result->status_code != 200)
    {
        // redirect
        if (Redirect(result->req, hapi_get_location(result->header), &g_hRequest))
        {
            return 0;
        }
        EnableDialog(hDlg, TRUE);
        MessageBoxFmt_(hDlg, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_REQUEST_ERROR, result->status_code);
        return 1;
    }

    OnRequestQuery(hapi_get_charset(result->header), hDlg);

    return 0;
}

static int OnRequestCharset(HWND hDlg)
{
    request_t req;
    char* query_format;
    char url[1024];
    char content[1024] = { 0 };
    char* encode;
    int sel;
    char *keyword = NULL;
    TCHAR text[256] = {0};
    http_charset_t charset;

    GetDlgItemText(hDlg, IDC_EDIT_QUERY_KEYWORD, text, 256);
    if (_tcslen(text) == 0)
    {
        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_EMPTY_KEYWORD, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    sel = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
    if (sel < 0 || sel >= MAX_BOOKSRC_COUNT)
    {
        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_SELECT_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    if (_header->book_sources[sel].query_charset != 0) // 0: auto
    {
        if (_header->book_sources[sel].query_charset == 1) // utf8
            charset = utf_8;
        else
            charset = gbk;
        return OnRequestQuery(charset, hDlg);
    }
    keyword = Utils::Utf16ToUtf8(text);
    if (_header->book_sources[sel].query_method == 0) // GET
    {
        query_format = _header->book_sources[sel].query_url;

        Utils::UrlEncode(keyword, &encode);
        sprintf(url, query_format, encode);
        Utils::UrlFree(encode);
    }
    else // POST
    {
        strcpy(url, _header->book_sources[sel].query_url);

        Utils::UrlEncode(keyword, &encode);
        sprintf(content, _header->book_sources[sel].query_params, encode);
        Utils::UrlFree(encode);
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
    req.param1 = hDlg;
    req.param2 = NULL;

    g_hRequest = hapi_request(&req);
    return 0;
}

static void EnableDialog(HWND hDlg, BOOL enable)
{
    TCHAR szQuery[256] = { 0 };
    TCHAR szStopQuery[256] = { 0 };
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

#endif
