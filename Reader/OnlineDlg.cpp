#include "stdafx.h"
#ifdef ENABLE_NETWORK
#include "OnlineDlg.h"
#include "resource.h"
#include "HtmlParser.h"
#include "httpclient.h"
#include "Utils.h"
#include "Jsondata.h"
#include <shellapi.h>
#include <commdlg.h>
#include <shlwapi.h>

static HINSTANCE g_Inst = NULL;
static HWND g_hWnd = NULL;
static BOOL g_Enable = TRUE;
static BOOL g_EnableSync = TRUE;
static req_handler_t g_hRequest = NULL;
static req_handler_t g_hRequestSync = NULL;
extern header_t* _header;
static int g_lastPos = 0;

extern void OnOpenOlBook(HWND, void*);
extern int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType);
int MessageBoxFmt_(HWND hWnd, UINT captionId, UINT uType, UINT formatId, ...);

static INT_PTR CALLBACK OnlineDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK OnBookSourceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static int OnRequestQuery(http_charset_t charset, HWND hDlg);
static int OnRequestCharset(HWND hDlg);
static int DownloadBooksrc(HWND hDlg);
static void EnableDialog(HWND hDlg, BOOL enable);
static void EnableDialog_Sync(HWND hDlg, BOOL enable);

void OpenBookSourceDlg(HINSTANCE hInst, HWND hWnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_BOOKSOURCE), hWnd, OnBookSourceDlgProc);
}

void OpenOnlineDlg(HINSTANCE hInst, HWND hWnd)
{
    g_Inst = hInst;
    g_hWnd = hWnd;
    DialogBox(hInst, MAKEINTRESOURCE(IDD_ONLINE), hWnd, OnlineDlgProc);
}

void DumpParseErrorFile(const char *html, int htmllen)
{
#if TEST_MODEL
    FILE *fp;
    if (html && htmllen > 0)
    {
        fp = fopen("dump.html", "wb");
        if (fp)
        {
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

BOOL Redirect(request_t *req, const char *url);

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
            iPos = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            idx = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
            if (iPos != -1 && idx != -1)
            {
                colnum = 1;
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
                ListView_GetItemText(hList, iPos, 0, param.book_name, 256);
                strcpy(param.main_page, Utils::Utf16ToUtf8(path));
                strcpy(param.host, _header->book_sources[idx].host);
                OnOpenOlBook(g_hWnd, &param);
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
            OpenBookSourceDlg(g_Inst, hDlg);
            break;
        case IDC_BUTTON_OL_QUERY:
            if (g_Enable)
            {
                if (_header->book_source_count == 0)
                {
                    if (IDYES == MessageBox_(hDlg, IDS_NOTEXIST_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_YESNO))
                    {
                        OpenBookSourceDlg(g_Inst, hDlg);
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

static void EnableBookStatus(HWND hDlg)
{
    int iPos;
    iPos = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), CB_GETCURSEL, 0, NULL);
    if (iPos == 0)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_STATUS), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_KEYWORD), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_STATUS), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_KEYWORD), TRUE);
    }
}

static void EnableQueryMethod(HWND hDlg)
{
    int iPos;
    iPos = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_METHOD), CB_GETCURSEL, 0, NULL);
    if (iPos == 0)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_POST), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_POST), TRUE);
    }
}

static void ClearBookSourceData(HWND hDlg)
{
    SetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_BS_HOST, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_BS_POST, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_BS_NAME, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_BS_MAINPAGE, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_BS_AUTHOR, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_BS_CTX, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_BS_STATUS, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_BS_KEYWORD, _T(""));
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), CB_SETCURSEL, 0, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_METHOD), CB_SETCURSEL, 0, NULL);
}

static void SetBookSourceDlgData(HWND hDlg, int idx, BOOL initList)
{
    int i;
    HWND hList;
    LV_COLUMN lvc = { 0 };
    LVITEM lvitem = { 0 };
    TCHAR szTitle[256];
    hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);
    if (initList && hList)
    {
        LoadString(g_Inst, IDS_TITLE, szTitle, 256);
        memset(&lvc, 0, sizeof(LV_COLUMN));
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.pszText = szTitle;
        lvc.cx = 120;
        SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);

        for (i = 0; i < _header->book_source_count; i++)
        {
            memset(&lvitem, 0, sizeof(LVITEM));
            lvitem.mask = LVIF_TEXT;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = i;
            lvitem.iSubItem = 0;
            lvitem.pszText = _header->book_sources[i].title;
            ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);
        }
    }
    if (idx >= 0 && idx < _header->book_source_count)
    {
        SetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, _header->book_sources[idx].title);
        SetDlgItemText(hDlg, IDC_EDIT_BS_HOST, Utils::Utf8ToUtf16(_header->book_sources[idx].host));
        SetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, Utils::Utf8ToUtf16(_header->book_sources[idx].query_url));
        SetDlgItemText(hDlg, IDC_EDIT_BS_POST, Utils::Utf8ToUtf16(_header->book_sources[idx].query_params));
        SetDlgItemText(hDlg, IDC_EDIT_BS_NAME, Utils::Utf8ToUtf16(_header->book_sources[idx].book_name_xpath));
        SetDlgItemText(hDlg, IDC_EDIT_BS_MAINPAGE, Utils::Utf8ToUtf16(_header->book_sources[idx].book_mainpage_xpath));
        SetDlgItemText(hDlg, IDC_EDIT_BS_AUTHOR, Utils::Utf8ToUtf16(_header->book_sources[idx].book_author_xpath));
        SetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, Utils::Utf8ToUtf16(_header->book_sources[idx].chapter_title_xpath));
        SetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, Utils::Utf8ToUtf16(_header->book_sources[idx].chapter_url_xpath));
        SetDlgItemText(hDlg, IDC_EDIT_BS_CTX, Utils::Utf8ToUtf16(_header->book_sources[idx].content_xpath));
        SetDlgItemText(hDlg, IDC_EDIT_BS_STATUS, Utils::Utf8ToUtf16(_header->book_sources[idx].book_status_xpath));
        SetDlgItemText(hDlg, IDC_EDIT_BS_KEYWORD, Utils::Utf8ToUtf16(_header->book_sources[idx].book_status_keyword));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), CB_SETCURSEL, _header->book_sources[idx].book_status_pos, NULL);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_METHOD), CB_SETCURSEL, _header->book_sources[idx].query_method, NULL);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_CHARSET), CB_SETCURSEL, _header->book_sources[idx].query_charset, NULL);
        ListView_SetItemState(hList, idx, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
    }
    else
    {
        ClearBookSourceData(hDlg);
    }
    EnableBookStatus(hDlg);
    EnableQueryMethod(hDlg);
}

static BOOL CheckBookSourceIsEmpty(HWND hDlg)
{
    TCHAR buf[1024];

    GetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, buf, 256);
    if (!buf[0])
    {
        MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
        return FALSE;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_HOST, buf, 256);
    if (!buf[0])
    {
        MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
        return FALSE;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, buf, 256);
    if (!buf[0])
    {
        MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
        return FALSE;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_NAME, buf, 256);
    if (!buf[0])
    {
        MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
        return FALSE;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_MAINPAGE, buf, 256);
    if (!buf[0])
    {
        MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
        return FALSE;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, buf, 256);
    if (!buf[0])
    {
        MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
        return FALSE;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, buf, 256);
    if (!buf[0])
    {
        MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
        return FALSE;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_CTX, buf, 256);
    if (!buf[0])
    {
        MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
        return FALSE;
    }
    if (SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), CB_GETCURSEL, 0, NULL) != 0)
    {
        GetDlgItemText(hDlg, IDC_EDIT_BS_STATUS, buf, 256);
        if (!buf[0])
        {
            MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
            return FALSE;
        }
        GetDlgItemText(hDlg, IDC_EDIT_BS_KEYWORD, buf, 256);
        if (!buf[0])
        {
            MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
            return FALSE;
        }
    }
    if (SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_METHOD), CB_GETCURSEL, 0, NULL) != 0)
    {
        GetDlgItemText(hDlg, IDC_EDIT_BS_POST, buf, 256);
        if (!buf[0])
        {
            MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL CheckBookSourceIsExist(HWND hDlg, int except)
{
    int i;
    TCHAR buf[1024];

    GetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, buf, 256);
    for (i = 0; i < _header->book_source_count; i++)
    {
        if (i != except && 0 == _tcscmp(buf, _header->book_sources[i].title))
        {
            MessageBox_(hDlg, IDS_BOOKSOURCE_EXIST, IDS_ERROR, MB_ICONERROR | MB_OK);
            return FALSE;
        }
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_HOST, buf, 256);
    for (i = 0; i < _header->book_source_count; i++)
    {
        if (i != except && 0 == _tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[i].host)))
        {
            MessageBox_(hDlg, IDS_BOOKSOURCE_EXIST, IDS_ERROR, MB_ICONERROR | MB_OK);
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL CheckBookSourceIsModify(HWND hDlg)
{
    int iPos;
    BOOL isModify = FALSE;
    HWND hList;
    TCHAR buf[1024];

    return TRUE; // disable this feature
    hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);
    iPos = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (iPos == -1 || iPos < 0 || iPos >= MAX_BOOKSRC_COUNT)
    {
        return TRUE;
    }

    GetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, buf, 256);
    if (_tcscmp(buf, _header->book_sources[iPos].title) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_HOST, buf, 256);
    if (_tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[iPos].host)) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, buf, 256);
    if (_tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[iPos].query_url)) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_POST, buf, 256);
    if (_tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[iPos].query_params)) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_NAME, buf, 256);
    if (_tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[iPos].book_name_xpath)) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_MAINPAGE, buf, 256);
    if (_tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[iPos].book_mainpage_xpath)) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_AUTHOR, buf, 256);
    if (_tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[iPos].book_author_xpath)) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, buf, 256);
    if (_tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[iPos].chapter_title_xpath)) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, buf, 256);
    if (_tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[iPos].chapter_url_xpath)) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_CTX, buf, 256);
    if (_tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[iPos].content_xpath)) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_STATUS, buf, 256);
    if (_tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[iPos].book_status_xpath)) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BS_KEYWORD, buf, 256);
    if (_tcscmp(buf, Utils::Utf8ToUtf16(_header->book_sources[iPos].book_status_keyword)) != 0)
    {
        isModify = TRUE;
        goto end;
    }
    if (SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), CB_GETCURSEL, 0, NULL) != _header->book_sources[iPos].book_status_pos)
    {
        isModify = TRUE;
        goto end;
    }
    if (SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_METHOD), CB_GETCURSEL, 0, NULL) != _header->book_sources[iPos].query_method)
    {
        isModify = TRUE;
        goto end;
    }
end:
    if (isModify)
    {
        if (IDNO == MessageBox_(hDlg, IDS_FILED_MODIFY, IDS_WARN, MB_ICONWARNING | MB_YESNO))
        {
            return FALSE;
        }
        SetBookSourceDlgData(hDlg, iPos, FALSE);
    }
    return TRUE;
}

static INT_PTR CALLBACK OnBookSourceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i;
    HWND hList;
    LV_COLUMN lvc = { 0 };
    LVITEM lvitem = { 0 };
    int iPos;
    int count;
    TCHAR buf[1024];
    switch (message)
    {
    case WM_INITDIALOG:
        g_hRequestSync = NULL;
        g_EnableSync = TRUE;
        hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);
        ListView_SetExtendedListViewStyleEx(hList, LVS_REPORT | LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_TWOCLICKACTIVATE);
        LoadString(g_Inst, IDS_NULL, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), CB_ADDSTRING, 0, (LPARAM)buf);
        LoadString(g_Inst, IDS_QUERYPAGE, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), CB_ADDSTRING, 0, (LPARAM)buf);
        LoadString(g_Inst, IDS_MAINPAGE, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), CB_ADDSTRING, 0, (LPARAM)buf);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), CB_SETCURSEL, 0, NULL);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_METHOD), CB_ADDSTRING, 0, (LPARAM)_T("GET"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_METHOD), CB_ADDSTRING, 0, (LPARAM)_T("POST"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_CHARSET), CB_ADDSTRING, 0, (LPARAM)_T("AUTO"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_CHARSET), CB_ADDSTRING, 0, (LPARAM)_T("UTF-8"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_CHARSET), CB_ADDSTRING, 0, (LPARAM)_T("GBK"));
        iPos = SendMessage(GetDlgItem(GetParent(hDlg), IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
        if (iPos < 0 || iPos >= _header->book_source_count)
            iPos = 0;
        SetBookSourceDlgData(hDlg, iPos, TRUE);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            // add item
            hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);

            // check is full
            count = ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_BOOKSRC));
            if (count >= MAX_BOOKSRC_COUNT)
            {
                MessageBoxFmt_(hDlg, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_BOOKSOURCE_FULL, MAX_BOOKSRC_COUNT);
                return (INT_PTR)FALSE;
            }

            // check empty
            if (!CheckBookSourceIsEmpty(hDlg))
            {
                return (INT_PTR)FALSE;
            }

            // check is exist
            if (!CheckBookSourceIsExist(hDlg, -1))
            {
                return (INT_PTR)FALSE;
            }

            // insert data
            GetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, _header->book_sources[_header->book_source_count].title, 256);
            GetDlgItemText(hDlg, IDC_EDIT_BS_HOST, buf, 1024);
            if (buf[_tcslen(buf) - 1] == _T('/')) // fixed host
            {
                buf[_tcslen(buf) - 1] = 0;
                SetDlgItemText(hDlg, IDC_EDIT_BS_HOST, buf);
            }
            strcpy(_header->book_sources[_header->book_source_count].host, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].query_url, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_POST, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].query_params, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_NAME, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].book_name_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_MAINPAGE, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].book_mainpage_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_AUTHOR, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].book_author_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].chapter_title_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].chapter_url_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CTX, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].content_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_STATUS, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].book_status_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_KEYWORD, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].book_status_keyword, Utils::Utf16ToUtf8(buf));
            _header->book_sources[_header->book_source_count].book_status_pos = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), CB_GETCURSEL, 0, NULL);
            _header->book_sources[_header->book_source_count].query_method = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_METHOD), CB_GETCURSEL, 0, NULL);
            _header->book_source_count++;

            // update list view
            memset(&lvitem, 0, sizeof(LVITEM));
            lvitem.mask = LVIF_TEXT;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = count;
            lvitem.iSubItem = 0;
            lvitem.pszText = _header->book_sources[count].title;
            ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);

            MessageBox_(hDlg, IDS_ADD_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
            break;
        case IDCANCEL:
            if (!CheckBookSourceIsModify(hDlg))
            {
                return (INT_PTR)FALSE;
            }
            if (g_hRequestSync)
            {
                hapi_cancel(g_hRequestSync);
            }
            g_hRequestSync = NULL;
            g_EnableSync = TRUE;
            // update parent combo
            SendMessage(GetDlgItem(GetParent(hDlg), IDC_COMBO_BS_LIST), CB_RESETCONTENT, 0, NULL);
            for (i = 0; i < _header->book_source_count; i++)
            {
                SendMessage(GetDlgItem(GetParent(hDlg), IDC_COMBO_BS_LIST), CB_ADDSTRING, 0, (LPARAM)_header->book_sources[i].title);
            }
            iPos = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_BOOKSRC), -1, LVNI_SELECTED);
            if (iPos < 0 || iPos >= _header->book_source_count)
                iPos = 0;
            SendMessage(GetDlgItem(GetParent(hDlg), IDC_COMBO_BS_LIST), CB_SETCURSEL, iPos, NULL);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_BUTTON_BS_SAVE:
            // save change
            hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);
            iPos = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (iPos == -1)
            {
                MessageBox_(hDlg, IDS_NO_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }

            // check empty
            if (!CheckBookSourceIsEmpty(hDlg))
            {
                return (INT_PTR)FALSE;
            }

            // check is exist
            if (!CheckBookSourceIsExist(hDlg, iPos))
            {
                return (INT_PTR)FALSE;
            }

            // save
            GetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, _header->book_sources[iPos].title, 256);
            GetDlgItemText(hDlg, IDC_EDIT_BS_HOST, buf, 1024);
            if (buf[_tcslen(buf) - 1] == _T('/')) // fixed host
            {
                buf[_tcslen(buf) - 1] = 0;
                SetDlgItemText(hDlg, IDC_EDIT_BS_HOST, buf);
            }
            strcpy(_header->book_sources[iPos].host, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, buf, 1024);
            strcpy(_header->book_sources[iPos].query_url, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_POST, buf, 1024);
            strcpy(_header->book_sources[iPos].query_params, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_NAME, buf, 1024);
            strcpy(_header->book_sources[iPos].book_name_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_MAINPAGE, buf, 1024);
            strcpy(_header->book_sources[iPos].book_mainpage_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_AUTHOR, buf, 1024);
            strcpy(_header->book_sources[iPos].book_author_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, buf, 1024);
            strcpy(_header->book_sources[iPos].chapter_title_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, buf, 1024);
            strcpy(_header->book_sources[iPos].chapter_url_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CTX, buf, 1024);
            strcpy(_header->book_sources[iPos].content_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_STATUS, buf, 1024);
            strcpy(_header->book_sources[iPos].book_status_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_KEYWORD, buf, 1024);
            strcpy(_header->book_sources[iPos].book_status_keyword, Utils::Utf16ToUtf8(buf));
            _header->book_sources[iPos].book_status_pos = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), CB_GETCURSEL, 0, NULL);
            _header->book_sources[iPos].query_method = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_METHOD), CB_GETCURSEL, 0, NULL);
            _header->book_sources[iPos].query_charset = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_CHARSET), CB_GETCURSEL, 0, NULL);

            // update list view
            memset(&lvitem, 0, sizeof(LVITEM));
            lvitem.mask = LVIF_TEXT;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = iPos;
            lvitem.iSubItem = 0;
            lvitem.pszText = _header->book_sources[iPos].title;
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);
            
            extern void Save(HWND);
            Save(g_hWnd);

            MessageBox_(hDlg, IDS_SAVE_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
            break;
        case IDC_BS_MANUAL:
            ShellExecute(NULL, _T("open"), _T("https://github.com/binbyu/Reader/blob/master/booksource.md"), NULL, NULL, SW_SHOWNORMAL);
            break;
        case IDC_COMBO_BS_STATUS:
            EnableBookStatus(hDlg);
            break;
        case IDC_COMBO_BS_METHOD:
            EnableQueryMethod(hDlg);
            break;
        case IDC_BS_IMPORT:
            {
                char* json;
                FILE* fp;
                int len;
                BOOL bSel;
                TCHAR szFileName[MAX_PATH] = { 0 };
                OPENFILENAME ofn = { 0 };

                if (!CheckBookSourceIsModify(hDlg))
                {
                    return (INT_PTR)FALSE;
                }

                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hDlg;
                ofn.lpstrFilter = _T("Files (*.json)\0*.json\0\0");
                ofn.lpstrInitialDir = NULL;
                ofn.lpstrFile = szFileName;
                ofn.nMaxFile = sizeof(szFileName) / sizeof(*szFileName);
                ofn.nFilterIndex = 0;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
                bSel = GetOpenFileName(&ofn);
                if (!bSel)
                {
                    break;
                }

                fp = _tfopen(szFileName, _T("rb"));
                if (!fp)
                {
                    MessageBox_(hDlg, IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
                    break;
                }
                fseek(fp, 0, SEEK_END);
                len = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                json = (char*)malloc(len+1);
                json[len] = 0;
                fread(json, 1, len, fp);
                fclose(fp);

                if (!import_book_source(json, _header->book_sources, &_header->book_source_count))
                {
                    MessageBox_(hDlg, IDS_IMPORT_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
                    break;
                }
                free(json);

                // update ui
                ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_BOOKSRC));
                SetBookSourceDlgData(hDlg, 0, TRUE);

                // save cache
                extern void Save(HWND);
                Save(g_hWnd);

                MessageBox_(hDlg, IDS_IMPORT_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
            }
            break;
        case IDC_BS_EXPORT:
            {
                char* json = NULL;
                FILE* fp;
                BOOL bSel;
                TCHAR szFileName[MAX_PATH] = { 0 };
                OPENFILENAME ofn = { 0 };

                if (!CheckBookSourceIsModify(hDlg))
                {
                    return (INT_PTR)FALSE;
                }

                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hDlg;
                ofn.lpstrFilter = _T("Files (*.json)\0*.json\0\0");
                ofn.lpstrInitialDir = NULL;
                ofn.lpstrFile = szFileName;
                ofn.nMaxFile = sizeof(szFileName) / sizeof(*szFileName);
                ofn.nFilterIndex = 0;
                ofn.Flags = OFN_OVERWRITEPROMPT;
                ofn.lpstrDefExt = _T(".json");
                bSel = GetSaveFileName(&ofn);
                if (!bSel)
                {
                    break;
                }

                if (!export_book_source(_header->book_sources, _header->book_source_count, &json))
                {
                    MessageBox_(hDlg, IDS_EXPORT_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
                    break;
                }

                fp = _tfopen(szFileName, _T("wb"));
                if (!fp)
                {
                    MessageBox_(hDlg, IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
                    break;
                }
                fwrite(json, 1, strlen(json), fp);
                fclose(fp);
                export_book_source_free(json);

                MessageBox_(hDlg, IDS_EXPORT_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
            }
            break;
        case IDC_BS_SYNC:
            {
                DownloadBooksrc(hDlg);
            }
            break;
        default:
            break;
        }
        break;
    case WM_NOTIFY:
        if (LOWORD(wParam) == IDC_LIST_BOOKSRC)
        {
            if (((LPNMHDR)lParam)->code == NM_CLICK)
            {
                iPos = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_BOOKSRC), -1, LVNI_SELECTED);
                if (iPos != -1)
                {
                    /*if (!CheckBookSourceIsModify(hDlg))
                    {
                        return (INT_PTR)FALSE;
                    }*/
                    SetBookSourceDlgData(hDlg, iPos, FALSE);
                }
                else
                {
                    ClearBookSourceData(hDlg);
                }
            }
        }
        break;
    case WM_CONTEXTMENU:
        hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);
        if (wParam == (WPARAM)hList)
        {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            iPos = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_BOOKSRC), -1, LVNI_SELECTED);
            if (iPos >= 0 && iPos < _header->book_source_count)
            {
                HMENU hMenu = CreatePopupMenu();
                if (hMenu)
                {
                    InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_BS_DEL, _T("Delete"));
                    int ret = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hList, NULL);
                    DestroyMenu(hMenu);
                    if (IDM_BS_DEL == ret)
                    {
                        // delete from data
                        for (int i = iPos + 1; i < _header->book_source_count; i++)
                        {
                            book_source_t* item_1 = &_header->book_sources[i];
                            book_source_t* item_2 = &_header->book_sources[i - 1];
                            memcpy(item_2, item_1, sizeof(book_source_t));
                        }
                        _header->book_source_count--;

                        // delete from list view
                        ListView_DeleteItem(hList, iPos);

                        SetBookSourceDlgData(hDlg, iPos, FALSE);
                    }
                }
            }
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
    int sel;
    char Url[1024] = { 0 };
    int needfree = 0;

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

    sel = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
    if (sel < 0 || sel >= MAX_BOOKSRC_COUNT)
    {
        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_SELECT_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    if (result->status_code != 200)
    {
        // redirect
        if (Redirect(result->req, hapi_get_location(result->header)))
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
    //if (hapi_get_charset(result->header) != utf_8)
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
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[sel].book_name_xpath, table_name, &cancel, true);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[sel].book_mainpage_xpath, table_url, &cancel);
    if (_header->book_sources[sel].book_author_xpath[0])
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[sel].book_author_xpath, table_author, &cancel, true);
    if (_header->book_sources[sel].book_status_pos == 1)
        HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[sel].book_status_xpath, table_status, &cancel, true);
    HtmlParser::Instance()->HtmlParseEnd(doc, ctx);

    // check value
    if (table_url.empty() || table_name.size() != table_url.size())
    {
        DumpParseErrorFile(html, htmllen);

        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_PARSE_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    if (needfree)
        free(html);

    hList = GetDlgItem(hDlg, IDC_LIST_QUERY);
    if (hList)
    {
        // book name
        LoadString(g_Inst, IDS_BOOK_NAME, colname, 256);
        col = 0;
        memset(&lvc, 0, sizeof(LV_COLUMN));
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.pszText = colname;
        lvc.cx = 120;
        SendMessage(hList, LVM_INSERTCOLUMN, col++, (LPARAM)&lvc);

        // book author
        if (!table_author.empty())
        {
            LoadString(g_Inst, IDS_AUTHOR, colname, 256);
            memset(&lvc, 0, sizeof(LV_COLUMN));
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            lvc.pszText = colname;
            lvc.cx = 60;
            SendMessage(hList, LVM_INSERTCOLUMN, col++, (LPARAM)&lvc);
        }

        // book status
        if (!table_status.empty())
        {
            LoadString(g_Inst, IDS_STATUS, colname, 256);
            memset(&lvc, 0, sizeof(LV_COLUMN));
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            lvc.pszText = colname;
            lvc.cx = 60;
            SendMessage(hList, LVM_INSERTCOLUMN, col++, (LPARAM)&lvc);
        }

        // mainpage
        LoadString(g_Inst, IDS_MAINPAGE, colname, 256);
        memset(&lvc, 0, sizeof(LV_COLUMN));
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.pszText = colname;
        lvc.cx = 180;
        SendMessage(hList, LVM_INSERTCOLUMN, col++, (LPARAM)&lvc);
        
        for (i = 0; i < (int)table_name.size(); i++)
        {
            col = 0;
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
            if (_strnicmp("http", table_url[i].c_str(), 4) != 0)
            {
                sprintf(Url, "%s%s", _header->book_sources[sel].host, table_url[i].c_str());
            }
            else
            {
                strcpy(Url, table_url[i].c_str());
            }
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
    if (sel < 0 || sel >= MAX_BOOKSRC_COUNT)
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
    req.param2 = NULL;

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
        if (Redirect(result->req, hapi_get_location(result->header)))
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

static unsigned int DownloadBooksrcCompleter(request_result_t* result)
{
    HWND hDlg = (HWND)result->param1;
    char* html = NULL;
    int htmllen = 0;
    int needfree = 0;

    g_hRequestSync = NULL;

    if (result->cancel)
    {
        EnableDialog_Sync(hDlg, TRUE);
        return 1;
    }

    if (result->errno_ != succ)
    {
        EnableDialog_Sync(hDlg, TRUE);
        MessageBox_(hDlg, IDS_SYNC_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    if (result->status_code != 200)
    {
        // redirect
        if (Redirect(result->req, hapi_get_location(result->header)))
        {
            return 0;
        }
        EnableDialog_Sync(hDlg, TRUE);
        MessageBox_(hDlg, IDS_SYNC_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    if (hapi_is_gzip(result->header))
    {
        needfree = 1;
        if (!Utils::gzipInflate((unsigned char*)result->body, result->bodylen, (unsigned char**)&html, &htmllen))
        {
            EnableDialog_Sync(hDlg, TRUE);
            MessageBox_(hDlg, IDS_SYNC_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
            return 1;
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

    // import book src
    if (!import_book_source(html, _header->book_sources, &_header->book_source_count))
    {
        EnableDialog_Sync(hDlg, TRUE);
        MessageBox_(hDlg, IDS_IMPORT_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    if (needfree)
        free(html);

    // update ui
    ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_BOOKSRC));
    SetBookSourceDlgData(hDlg, 0, TRUE);

    // save cache
    extern void Save(HWND);
    Save(g_hWnd);

    EnableDialog_Sync(hDlg, TRUE);
    MessageBox_(hDlg, IDS_SYNC_SUCC, IDS_ERROR, MB_OK);

    return 0;
}

static int DownloadBooksrc(HWND hDlg)
{
    request_t req;

    if (!g_EnableSync)
    {
        if (g_hRequestSync)
            hapi_cancel(g_hRequestSync);
        return 0;
    }

    if (_header->book_source_count > 0)
    {
        if (IDYES != MessageBox_(hDlg, IDS_LOST_WARN, IDS_WARN, MB_ICONWARNING | MB_YESNO))
            return 0;
    }

    EnableDialog_Sync(hDlg, FALSE);

    // do request    
    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = "https://raw.githubusercontent.com/binbyu/Reader/master/bs.json";
    req.completer = DownloadBooksrcCompleter;
    req.param1 = hDlg;
    req.param2 = NULL;

    g_hRequestSync = hapi_request(&req);
    return 0;
}

static void EnableDialog(HWND hDlg, BOOL enable)
{
    TCHAR szQuery[256] = { 0 };
    TCHAR szStopQuery[256] = { 0 };
    LoadString(g_Inst, IDS_QUERY, szQuery, 256);
    LoadString(g_Inst, IDS_STOP_QUERY, szStopQuery, 256);
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

static void EnableDialog_Sync(HWND hDlg, BOOL enable)
{
    TCHAR szSync[256] = { 0 };
    TCHAR szStopSync[256] = { 0 };
    LoadString(g_Inst, IDS_AUTO_SYNC, szSync, 256);
    LoadString(g_Inst, IDS_STOP_SYNC, szStopSync, 256);
    EnableWindow(GetDlgItem(hDlg, IDC_LIST_BOOKSRC), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_TITLE), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_HOST), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BS_METHOD), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_QUERY), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_POST), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BS_CHARSET), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_NAME), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_MAINPAGE), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_AUTHOR), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_CPT_TITLE), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_CPT_URL), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_CTX), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BS_STATUS), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_STATUS), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BS_KEYWORD), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_BS_EXPORT), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_BS_IMPORT), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_BS_MANUAL), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BS_SAVE), enable);
    EnableWindow(GetDlgItem(hDlg, IDOK), enable);
    g_EnableSync = enable;
    if (enable)
        SetDlgItemText(hDlg, IDC_BS_SYNC, szSync);
    else
        SetDlgItemText(hDlg, IDC_BS_SYNC, szStopSync);
}

BOOL Redirect(request_t *r, const char *url)
{
    request_t req;
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

    memset(&req, 0, sizeof(request_t));
    req.method = r->method;
    req.url = (char *)url;
    req.content = r->content;
    req.content_length = r->content_length;
    req.completer = r->completer;
    req.param1 = r->param1;
    req.param2 = r->param2;

    g_hRequest = hapi_request(&req);
    return TRUE;
}

#endif
