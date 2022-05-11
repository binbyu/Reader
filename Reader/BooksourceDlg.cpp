#ifdef ENABLE_NETWORK
#include "framework.h"
#include "BooksourceDlg.h"
#include "resource.h"
#include "types.h"
#include "Utils.h"
#include "https.h"
#include "Jsondata.h"
#include <shellapi.h>
#include <commdlg.h>
#include <stdio.h>
#include <regex>

extern header_t *_header;
extern HWND _hWnd;
extern HINSTANCE hInst;
extern void Save(HWND);
extern int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType);
extern int MessageBoxFmt_(HWND hWnd, UINT captionId, UINT uType, UINT formatId, ...);
extern void ReloadBookSourceCombobox(HWND hDlg, int iPos);

static BOOL g_EnableSync = TRUE;
static req_handler_t g_hRequestSync = NULL;

static INT_PTR CALLBACK BS_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void BS_OpenDlg(HWND hWnd)
{
	DialogBox(hInst, MAKEINTRESOURCE(IDD_BOOKSOURCE), hWnd, BS_DlgProc);
}

static void _enable_content_filter(HWND hDlg)
{
    LRESULT iPos;
    iPos = SendMessage(GetDlgItem(hDlg, IDC_COMBO_FILTER), CB_GETCURSEL, 0, NULL);
    if (iPos == 0)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FILTER), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FILTER), TRUE);
    }
}

static void _enable_query_method(HWND hDlg)
{
    LRESULT iPos;
    iPos = SendMessage(GetDlgItem(hDlg, IDC_COMBO_METHOD), CB_GETCURSEL, 0, NULL);
    if (iPos == 0)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_POST), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_POST), TRUE);
    }
}

static void _enable_chapter_page(HWND hDlg)
{
    LRESULT iPos;
    iPos = SendMessage(GetDlgItem(hDlg, IDC_COMBO_LISTPAGE), CB_GETCURSEL, 0, NULL);
    if (iPos == 0)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_LISTPAGE), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_LISTPAGE), TRUE);
    }
}

static void _enable_content_next(HWND hDlg)
{
    LRESULT iPos;
    iPos = SendMessage(GetDlgItem(hDlg, IDC_COMBO_CTX_NEXT), CB_GETCURSEL, 0, NULL);
    if (iPos == 0)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CTX_NEXT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CTX_KEYWORD), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CTX_KEYWORD_TXT), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CTX_NEXT), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CTX_KEYWORD), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CTX_KEYWORD_TXT), TRUE);
    }
}

static void _enable_chapter_next(HWND hDlg)
{
    LRESULT iPos;
    iPos = SendMessage(GetDlgItem(hDlg, IDC_COMBO_CPT_NEXT), CB_GETCURSEL, 0, NULL);
    if (iPos == 0)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_NEXT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD_TXT), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_NEXT), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD_TXT), TRUE);
    }
}

static void _clear_ui(HWND hDlg)
{
    SetDlgItemText(hDlg, IDC_EDIT_TITLE, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_HOST, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_QUERY, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_POST, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_NAME, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_MAINPAGE, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_AUTHOR, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_LISTPAGE, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_CPT_TITLE, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_CPT_URL, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_CPT_NEXT, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_CPT_KEYWORD, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_CPT_KEYWORD_TXT, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_CTX, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_CTX_NEXT, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_CTX_KEYWORD, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_CTX_KEYWORD_TXT, _T(""));
    SetDlgItemText(hDlg, IDC_EDIT_FILTER, _T(""));
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_METHOD), CB_SETCURSEL, 0, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_CHARSET), CB_SETCURSEL, 0, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_LISTPAGE), CB_SETCURSEL, 0, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_CPT_NEXT), CB_SETCURSEL, 0, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_CTX_NEXT), CB_SETCURSEL, 0, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_FILTER), CB_SETCURSEL, 0, NULL);
}

static void _set_data_to_ui(HWND hDlg, const book_source_t *data)
{
    SetDlgItemText(hDlg, IDC_EDIT_TITLE, data->title);
    SetDlgItemText(hDlg, IDC_EDIT_HOST, Utf8ToUtf16(data->host));
    SetDlgItemText(hDlg, IDC_EDIT_QUERY, Utf8ToUtf16(data->query_url));
    SetDlgItemText(hDlg, IDC_EDIT_POST, Utf8ToUtf16(data->query_params));
    SetDlgItemText(hDlg, IDC_EDIT_NAME, Utf8ToUtf16(data->book_name_xpath));
    SetDlgItemText(hDlg, IDC_EDIT_MAINPAGE, Utf8ToUtf16(data->book_mainpage_xpath));
    SetDlgItemText(hDlg, IDC_EDIT_AUTHOR, Utf8ToUtf16(data->book_author_xpath));
    SetDlgItemText(hDlg, IDC_EDIT_LISTPAGE, Utf8ToUtf16(data->chapter_page_xpath));
    SetDlgItemText(hDlg, IDC_EDIT_CPT_TITLE, Utf8ToUtf16(data->chapter_title_xpath));
    SetDlgItemText(hDlg, IDC_EDIT_CPT_URL, Utf8ToUtf16(data->chapter_url_xpath));
    SetDlgItemText(hDlg, IDC_EDIT_CPT_NEXT, Utf8ToUtf16(data->chapter_next_url_xpath));
    SetDlgItemText(hDlg, IDC_EDIT_CPT_KEYWORD, Utf8ToUtf16(data->chapter_next_keyword_xpath));
    SetDlgItemText(hDlg, IDC_EDIT_CPT_KEYWORD_TXT, Utf8ToUtf16(data->chapter_next_keyword));
    SetDlgItemText(hDlg, IDC_EDIT_CTX, Utf8ToUtf16(data->content_xpath));
    SetDlgItemText(hDlg, IDC_EDIT_CTX_NEXT, Utf8ToUtf16(data->content_next_url_xpath));
    SetDlgItemText(hDlg, IDC_EDIT_CTX_KEYWORD, Utf8ToUtf16(data->content_next_keyword_xpath));
    SetDlgItemText(hDlg, IDC_EDIT_CTX_KEYWORD_TXT, Utf8ToUtf16(data->content_next_keyword));
    SetDlgItemText(hDlg, IDC_EDIT_FILTER, data->content_filter_keyword);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_METHOD), CB_SETCURSEL, data->query_method, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_CHARSET), CB_SETCURSEL, data->query_charset, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_LISTPAGE), CB_SETCURSEL, data->enable_chapter_page, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_CPT_NEXT), CB_SETCURSEL, data->enable_chapter_next, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_CTX_NEXT), CB_SETCURSEL, data->enable_content_next, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_FILTER), CB_SETCURSEL, data->content_filter_type, NULL);
}

static void _get_data_from_ui(HWND hDlg, book_source_t *data)
{
    TCHAR buf[1024];
    size_t i,j=0,len;
    GetDlgItemText(hDlg, IDC_EDIT_TITLE, data->title, 256);
    GetDlgItemText(hDlg, IDC_EDIT_HOST, buf, 1023);
    if (buf[_tcslen(buf) - 1] == _T('/')) // fixed host
    {
        buf[_tcslen(buf) - 1] = 0;
        SetDlgItemText(hDlg, IDC_EDIT_HOST, buf);
    }
    strcpy(data->host, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_QUERY, buf, 1023);
    strcpy(data->query_url, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_POST, buf, 1023);
    strcpy(data->query_params, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_NAME, buf, 1023);
    strcpy(data->book_name_xpath, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_MAINPAGE, buf, 1023);
    strcpy(data->book_mainpage_xpath, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_AUTHOR, buf, 1023);
    strcpy(data->book_author_xpath, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_LISTPAGE, buf, 1023);
    strcpy(data->chapter_page_xpath, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_CPT_TITLE, buf, 1023);
    strcpy(data->chapter_title_xpath, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_CPT_URL, buf, 1023);
    strcpy(data->chapter_url_xpath, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_CPT_NEXT, buf, 1023);
    strcpy(data->chapter_next_url_xpath, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_CPT_KEYWORD, buf, 1023);
    strcpy(data->chapter_next_keyword_xpath, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_CPT_KEYWORD_TXT, buf, 1023);
    strcpy(data->chapter_next_keyword, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_CTX, buf, 1023);
    strcpy(data->content_xpath, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_CTX_NEXT, buf, 1023);
    strcpy(data->content_next_url_xpath, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_CTX_KEYWORD, buf, 1023);
    strcpy(data->content_next_keyword_xpath, Utf16ToUtf8(buf));
    GetDlgItemText(hDlg, IDC_EDIT_CTX_KEYWORD_TXT, buf, 1023);
    strcpy(data->content_next_keyword, Utf16ToUtf8(buf));
    data->content_filter_type = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBO_FILTER), CB_GETCURSEL, 0, NULL);
    // <!-- format filter keyword, \r\n -> \n
    GetDlgItemText(hDlg, IDC_EDIT_FILTER, buf, 1023);
    if (data->content_filter_type == 1)
    {
        
        len = _tcslen(buf);
        for (i = 0; i < len; i++)
        {
            if (buf[i] == _T('\n') && i > 0 && buf[i - 1] == _T('\r'))
            {
                buf[i - 1] = _T('\n');
                continue;
            }
            data->content_filter_keyword[j++] = buf[i];
        }
        data->content_filter_keyword[j] = 0;
    }
    else
    {
        _tcscpy(data->content_filter_keyword, buf);
    }
    // --!>
    data->query_method = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBO_METHOD), CB_GETCURSEL, 0, NULL);
    data->query_charset = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBO_CHARSET), CB_GETCURSEL, 0, NULL);
    data->enable_chapter_page = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBO_LISTPAGE), CB_GETCURSEL, 0, NULL);
    data->enable_chapter_next = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBO_CPT_NEXT), CB_GETCURSEL, 0, NULL);
    data->enable_content_next = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBO_CTX_NEXT), CB_GETCURSEL, 0, NULL);
}

static void _load_ui(HWND hDlg, int idx, BOOL initList)
{
    int i, colnum;
    HWND hList, hHeader;
    LV_COLUMN lvc = { 0 };
    LVITEM lvitem = { 0 };
    TCHAR szTitle[256];
    hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);
    if (initList && hList)
    {
        hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
        colnum = (int)SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0);
        for (i = colnum - 1; i >= 0; i--)
            SendMessage(hList, LVM_DELETECOLUMN, i, 0);

        LoadString(hInst, IDS_TITLE, szTitle, 256);
        memset(&lvc, 0, sizeof(LV_COLUMN));
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.pszText = szTitle;
        lvc.cx = 146;
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
        _set_data_to_ui(hDlg, &_header->book_sources[idx]);
        ListView_SetItemState(hList, idx, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
    }
    else
    {
        _clear_ui(hDlg);
    }
    _enable_query_method(hDlg);
    _enable_chapter_page(hDlg);
    _enable_chapter_next(hDlg);
    _enable_content_next(hDlg);
    _enable_content_filter(hDlg);
}

static BOOL _check_is_empty(HWND hDlg, book_source_t *data)
{
    book_source_t *p_temp = NULL;

    if (!data)
    {
        p_temp = (book_source_t*)malloc(sizeof(book_source_t));
        memset(p_temp, 0, sizeof(book_source_t));
        _get_data_from_ui(hDlg, p_temp);
        data = p_temp;
    }
    if (!data->title[0])
        goto _yes;
    if (!data->host[0])
        goto _yes;
    if (!data->query_url[0])
        goto _yes;
    if (!data->book_name_xpath[0])
        goto _yes;
    if (!data->book_mainpage_xpath[0])
        goto _yes;
    //if (!data->book_author_xpath[0])
    //    goto _yes;
    if (!data->chapter_title_xpath[0])
        goto _yes;
    if (!data->chapter_url_xpath[0])
        goto _yes;
    if (!data->content_xpath[0])
        goto _yes;
    if (data->query_method != 0)
    {
        if (!data->query_params[0])
            goto _yes;
    }
    if (data->enable_chapter_page)
    {
        if (!data->chapter_page_xpath[0])
            goto _yes;
    }
    if (data->enable_content_next)
    {
        if (!data->content_next_url_xpath[0])
            goto _yes;
        if (!data->content_next_keyword_xpath[0])
            goto _yes;
        if (!data->content_next_keyword[0])
            goto _yes;
    }
    if (data->content_filter_type != 0)
    {
        if (!data->content_filter_keyword[0])
            goto _yes;
    }
    if (p_temp)
        free(p_temp);
    return FALSE;

_yes:
    if (p_temp)
        free(p_temp);
    MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
    return TRUE;
}

static BOOL _check_is_valid(HWND hDlg, book_source_t* data)
{
    book_source_t* p_temp = NULL;
    std::wregex* e = NULL;

    if (!data)
    {
        p_temp = (book_source_t*)malloc(sizeof(book_source_t));
        memset(p_temp, 0, sizeof(book_source_t));
        _get_data_from_ui(hDlg, p_temp);
        data = p_temp;
    }

    if (data->content_filter_type == 2)
    {
        try
        {
            e = new std::wregex(data->content_filter_keyword);
        }
        catch (...)
        {
            if (e)
            {
                delete e;
            }
            if (p_temp)
                free(p_temp);
            MessageBox_(hDlg, IDS_INVALID_REGEX, IDS_ERROR, MB_ICONERROR | MB_OK);
            return FALSE;
        }
    }
    if (p_temp)
        free(p_temp);
    return TRUE;
}

static BOOL _check_is_exist(HWND hDlg, int except, book_source_t *data)
{
    int i;
    book_source_t *p_temp = NULL;
    if (!data)
    {
        p_temp = (book_source_t*)malloc(sizeof(book_source_t));
        memset(p_temp, 0, sizeof(book_source_t));
        _get_data_from_ui(hDlg, p_temp);
        data = p_temp;
    }

    for (i = 0; i < _header->book_source_count; i++)
    {
        if (i != except && 0 == _tcscmp(data->title, _header->book_sources[i].title))
            goto _yes;
    }
    for (i = 0; i < _header->book_source_count; i++)
    {
        if (i != except && 0 == strcmp(data->host, _header->book_sources[i].host))
            goto _yes;
    }
    if (p_temp)
        free(p_temp);
    return FALSE;

_yes:
    if (p_temp)
        free(p_temp);
    MessageBox_(hDlg, IDS_BOOKSOURCE_EXIST, IDS_ERROR, MB_ICONERROR | MB_OK);
    return TRUE;
}

static BOOL _check_is_modify(HWND hDlg, int idx, book_source_t *data)
{
    BOOL is_modify;
    book_source_t *p_temp = NULL;
    if (!data)
    {
        p_temp = (book_source_t*)malloc(sizeof(book_source_t));
        memset(p_temp, 0, sizeof(book_source_t));
        _get_data_from_ui(hDlg, p_temp);
        data = p_temp;
    }

    is_modify = 0 != memcmp(data, &_header->book_sources[idx], sizeof(book_source_t));
    if (p_temp)
        free(p_temp);
    return is_modify;
}

static void _backup_curn_bsconfig(void)
{
    char *json = NULL;
    FILE *fp = NULL;
    TCHAR file_name[MAX_PATH] = {0};
    const TCHAR *bak_name = _T(".bs_bak.json");
    size_t i;

    if (_header->book_source_count == 0)
        return;

    if (export_book_source(_header->book_sources, _header->book_source_count, &json))
    {
        GetModuleFileName(NULL, file_name, sizeof(TCHAR) * (MAX_PATH - 1));
        for (i = _tcslen(file_name) - 1; i >= 0; i--)
        {
            if (file_name[i] == _T('\\'))
            {
                memcpy(&file_name[i + 1], bak_name, (_tcslen(bak_name) + 1) * sizeof(TCHAR));
                break;
            }
        }

        fp = _tfopen(file_name, _T("wb"));
        if (fp)
        {
            fwrite(json, 1, strlen(json), fp);
            fclose(fp);
        }
        SetFileAttributes(file_name, FILE_ATTRIBUTE_HIDDEN);
    }

    export_book_source_free(json);
}

static BOOL _merge_bsconfig(HWND hDlg, const char *json)
{
    int book_source_count;
    book_source_t *book_sources = NULL;
    int i, j, ret;
    int found;

    book_sources = (book_source_t*)malloc(sizeof(book_source_t) * MAX_BOOKSRC_COUNT);
    memset(book_sources, 0, sizeof(book_source_t) * MAX_BOOKSRC_COUNT);
    book_source_count = 0;
    if (!import_book_source(json, book_sources, &book_source_count))
    {
        free(book_sources);
        return FALSE;
    }
    // no changed
    if (book_source_count == _header->book_source_count && 0 == memcmp(book_sources, _header->book_sources, sizeof(book_sources)))
    {
        free(book_sources);
        return TRUE;
    }
    // do check
    for (i = 0; i < book_source_count; i++)
    {
        found = 0;
        for (j = 0; j < _header->book_source_count; j++)
        {
            if (0 == memcmp(&book_sources[i], &_header->book_sources[j], sizeof(book_source_t)))
            {
                found = 1;
                break;
            }
            if (0 == strcmp(book_sources[i].host, _header->book_sources[j].host))
            {
                found = 1;
                ret = MessageBoxFmt_(hDlg, IDS_WARN, MB_ICONINFORMATION | MB_YESNO, IDS_BS_EXIST_TIP, _header->book_sources[j].title);
                if (IDYES == ret)
                {
                    // replace
                    memcpy(&_header->book_sources[j], &book_sources[i], sizeof(book_source_t));
                }
                else if (IDNO == ret)
                {
                    // ignore, keep old config
                }
                else
                {
                    // exit
                    free(book_sources);
                    return TRUE;
                }
                break;
            }
        }
        if (found == 0)
        {
            // add new one
            if (_header->book_source_count < MAX_BOOKSRC_COUNT)
            {
                memcpy(&_header->book_sources[_header->book_source_count], &book_sources[i], sizeof(book_source_t));
                _header->book_source_count++;
            }
        }
    }
    free(book_sources);
    return TRUE;
}

static void EnableDialog_Sync(HWND hDlg, BOOL enable)
{
    TCHAR szSync[256] = { 0 };
    TCHAR szStopSync[256] = { 0 };
    book_source_t *p_temp = NULL;
    LoadString(hInst, IDS_AUTO_SYNC, szSync, 256);
    LoadString(hInst, IDS_STOP_SYNC, szStopSync, 256);
   
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_TITLE), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HOST), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_QUERY), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_POST), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NAME), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MAINPAGE), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_AUTHOR), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_LISTPAGE), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_TITLE), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_URL), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_NEXT), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD_TXT), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CTX), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CTX_NEXT), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CTX_KEYWORD), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CTX_KEYWORD_TXT), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FILTER), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_METHOD), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_CHARSET), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_LISTPAGE), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_CPT_NEXT), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_CTX_NEXT), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_FILTER), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_LIST_BOOKSRC), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EXPORT), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_IMPORT), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_MANUAL), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_SAVE), enable);
    EnableWindow(GetDlgItem(hDlg, IDOK), enable);

    if (enable)
    {
        p_temp = (book_source_t*)malloc(sizeof(book_source_t));
        memset(p_temp, 0, sizeof(book_source_t));
        _get_data_from_ui(hDlg, p_temp);
        if (p_temp)
            free(p_temp);
        p_temp = NULL;

        _enable_query_method(hDlg);
        _enable_chapter_page(hDlg);
        _enable_chapter_next(hDlg);
        _enable_content_next(hDlg);
        _enable_content_filter(hDlg);
    }

    g_EnableSync = enable;
    if (enable)
        SetDlgItemText(hDlg, IDC_SYNC, szSync);
    else
        SetDlgItemText(hDlg, IDC_SYNC, szStopSync);
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
        EnableDialog_Sync(hDlg, TRUE);
        MessageBox_(hDlg, IDS_SYNC_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    html = result->body;
    htmllen = result->bodylen;

#if 0
    //if (hapi_get_charset(result->header) != utf_8)
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
        html = utf8buf;
        htmllen = utf8len;
        needfree = 1;
    }

    // backup current bs config
    _backup_curn_bsconfig();

    // import book src
    if (!import_book_source(html, _header->book_sources, &_header->book_source_count))
    {
        EnableDialog_Sync(hDlg, TRUE);
        MessageBox_(hDlg, IDS_IMPORT_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        if (needfree == 1)
            free(html);
        return 1;
    }

    // update ui
    ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_BOOKSRC));
    _load_ui(hDlg, 0, TRUE);

    // save cache
    Save(_hWnd);

    EnableDialog_Sync(hDlg, TRUE);
    MessageBox_(hDlg, IDS_SYNC_SUCC, IDS_ERROR, MB_OK);
    if (needfree == 1)
        free(html);
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
    req.url = (char*)"https://raw.githubusercontent.com/binbyu/Reader/master/bs.json";
    req.completer = DownloadBooksrcCompleter;
    req.param1 = hDlg;
    req.param2 = NULL;

    g_hRequestSync = hapi_request(&req);
    return 0;
}

static INT_PTR CALLBACK BS_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hList;
    LV_COLUMN lvc = { 0 };
    LVITEM lvitem = { 0 };
    int iPos;
    TCHAR buf[1024];
    static int s_iLastPos = -1;
    book_source_t *p_temp = NULL;

    switch (message)
    {
    case WM_INITDIALOG:
        g_hRequestSync = NULL;
        g_EnableSync = TRUE;
        hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);
        ListView_SetExtendedListViewStyleEx(hList, LVS_REPORT | LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_TWOCLICKACTIVATE);
        LoadString(hInst, IDS_NULL, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_FILTER), CB_ADDSTRING, 0, (LPARAM)buf);
        LoadString(hInst, IDS_KEYWORD, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_FILTER), CB_ADDSTRING, 0, (LPARAM)buf);
        LoadString(hInst, IDS_REGEX, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_FILTER), CB_ADDSTRING, 0, (LPARAM)buf);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_METHOD), CB_ADDSTRING, 0, (LPARAM)_T("GET"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_METHOD), CB_ADDSTRING, 0, (LPARAM)_T("POST"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_CHARSET), CB_ADDSTRING, 0, (LPARAM)_T("AUTO"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_CHARSET), CB_ADDSTRING, 0, (LPARAM)_T("UTF-8"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_CHARSET), CB_ADDSTRING, 0, (LPARAM)_T("GBK"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_LISTPAGE), CB_ADDSTRING, 0, (LPARAM)_T("Disable"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_LISTPAGE), CB_ADDSTRING, 0, (LPARAM)_T("Enable"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_CPT_NEXT), CB_ADDSTRING, 0, (LPARAM)_T("Disable"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_CPT_NEXT), CB_ADDSTRING, 0, (LPARAM)_T("Enable"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_CTX_NEXT), CB_ADDSTRING, 0, (LPARAM)_T("Disable"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_CTX_NEXT), CB_ADDSTRING, 0, (LPARAM)_T("Enable"));
        iPos = (int)SendMessage(GetDlgItem(GetParent(hDlg), IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
        if (iPos < 0 || iPos >= _header->book_source_count)
            iPos = 0;
        _load_ui(hDlg, iPos, TRUE);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            // add item
            hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);

            // check is full
            if (_header->book_source_count >= MAX_BOOKSRC_COUNT)
            {
                MessageBoxFmt_(hDlg, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_BOOKSOURCE_FULL, MAX_BOOKSRC_COUNT);
                return (INT_PTR)FALSE;
            }

            if (!p_temp)
                p_temp = (book_source_t*)malloc(sizeof(book_source_t));
            memset(p_temp, 0, sizeof(book_source_t));
            _get_data_from_ui(hDlg, p_temp);

            // check empty
            if (_check_is_empty(hDlg, p_temp))
            {
                free(p_temp);
                return (INT_PTR)FALSE;
            }

            // check valid
            if (!_check_is_valid(hDlg, p_temp))
            {
                free(p_temp);
                return (INT_PTR)FALSE;
            }

            // check is exist
            if (_check_is_exist(hDlg, -1, p_temp))
            {
                free(p_temp);
                return (INT_PTR)FALSE;
            }

            // insert data
            iPos = _header->book_source_count;
            memcpy(&_header->book_sources[iPos], p_temp, sizeof(book_source_t));
            _header->book_source_count++;

            free(p_temp);
            p_temp = NULL;

            // update list view
            memset(&lvitem, 0, sizeof(LVITEM));
            lvitem.mask = LVIF_TEXT;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = iPos;
            lvitem.iSubItem = 0;
            lvitem.pszText = _header->book_sources[iPos].title;
            ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);
            ListView_SetItemState(hList, iPos, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);

            // save cache
            Save(_hWnd);

            MessageBox_(hDlg, IDS_ADD_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
            break;
        case IDCANCEL:
#if 0
            if (_check_is_modify(hDlg, NULL))
            {
                return (INT_PTR)FALSE;
            }
#endif
            if (g_hRequestSync)
            {
                hapi_cancel(g_hRequestSync);
            }
            g_hRequestSync = NULL;
            g_EnableSync = TRUE;
            // update parent combo
            iPos = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_BOOKSRC), -1, LVNI_SELECTED);
            ReloadBookSourceCombobox(GetParent(hDlg), iPos);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_SAVE:
            // save change
            hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);
            iPos = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (iPos == -1)
            {
                MessageBox_(hDlg, IDS_NO_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }

            if (!p_temp)
                p_temp = (book_source_t*)malloc(sizeof(book_source_t));
            memset(p_temp, 0, sizeof(book_source_t));
            _get_data_from_ui(hDlg, p_temp);

            // check empty
            if (_check_is_empty(hDlg, p_temp))
            {
                free(p_temp);
                return (INT_PTR)FALSE;
            }

            // check valid
            if (!_check_is_valid(hDlg, p_temp))
            {
                free(p_temp);
                return (INT_PTR)FALSE;
            }

            // check is exist
            if (_check_is_exist(hDlg, iPos, p_temp))
            {
                free(p_temp);
                return (INT_PTR)FALSE;
            }

            // check is modify
            if (!_check_is_modify(hDlg, iPos, p_temp))
            {
                free(p_temp);
                MessageBox_(hDlg, IDS_SAVE_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
                return (INT_PTR)FALSE;
            }

            // check if want to add new
            if (0 != _tcscmp(_header->book_sources[iPos].title, p_temp->title)
                && 0 != strcmp(_header->book_sources[iPos].host, p_temp->host))
            {
                if (IDYES != MessageBoxFmt_(hDlg, IDS_WARN, MB_ICONINFORMATION | MB_YESNO, IDS_BS_SAVE_TIP, _header->book_sources[iPos].title))
                {
                    free(p_temp);
                    return (INT_PTR)FALSE;
                }
            }

            // save
            memcpy(&_header->book_sources[iPos], p_temp, sizeof(book_source_t));

            free(p_temp);
            p_temp = NULL;

            // update list view
            memset(&lvitem, 0, sizeof(LVITEM));
            lvitem.mask = LVIF_TEXT;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = iPos;
            lvitem.iSubItem = 0;
            lvitem.pszText = _header->book_sources[iPos].title;
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);

            // save cache
            Save(_hWnd);

            MessageBox_(hDlg, IDS_SAVE_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
            break;
        case IDC_MANUAL:
            ShellExecute(NULL, _T("open"), _T("https://github.com/binbyu/Reader/blob/master/doc/bs.md"), NULL, NULL, SW_SHOWNORMAL);
            break;
        case IDC_COMBO_METHOD:
            _enable_query_method(hDlg);
            break;
        case IDC_COMBO_LISTPAGE:
            _enable_chapter_page(hDlg);
            break;
        case IDC_COMBO_CTX_NEXT:
            _enable_content_next(hDlg);
            break;
        case IDC_COMBO_CPT_NEXT:
            _enable_chapter_next(hDlg);
            break;
        case IDC_COMBO_FILTER:
            _enable_content_filter(hDlg);
            break;        
        case IDC_IMPORT:
        {
            char* json;
            FILE* fp;
            int len;
            BOOL bSel;
            TCHAR szFileName[MAX_PATH] = { 0 };
            OPENFILENAME ofn = { 0 };

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

            // backup current bs config
            _backup_curn_bsconfig();

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

#if 1
            if (!_merge_bsconfig(hDlg, json))
#else
            if (!import_book_source(json, _header->book_sources, &_header->book_source_count))
#endif
            {
                MessageBox_(hDlg, IDS_IMPORT_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            free(json);

            // update ui
            ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_BOOKSRC));
            _load_ui(hDlg, 0, TRUE);

            // save cache
            Save(_hWnd);

            MessageBox_(hDlg, IDS_IMPORT_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
        }
        break;
        case IDC_EXPORT:
        {
            char* json = NULL;
            FILE* fp;
            BOOL bSel;
            TCHAR szFileName[MAX_PATH] = { 0 };
            OPENFILENAME ofn = { 0 };

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
        case IDC_SYNC:
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
            if (((LPNMHDR)lParam)->code == NM_CLICK
                || ((LPNMHDR)lParam)->code == NM_DBLCLK)
            {
                hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);
                iPos = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                if (iPos != -1)
                {
                    s_iLastPos = iPos;
                    /*if (!CheckBookSourceIsModify(hDlg))
                    {
                    return (INT_PTR)FALSE;
                    }*/
                    _load_ui(hDlg, iPos, FALSE);
                }
                else
                {
                    ListView_SetItemState(hList, s_iLastPos, LVIS_SELECTED, LVIS_SELECTED);
                    return (INT_PTR)FALSE;
                    //ClearBookSourceData(hDlg);
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
                s_iLastPos = iPos;
                HMENU hMenu = CreatePopupMenu();
                if (hMenu)
                {
                    TCHAR str[256] = { 0 };
                    LoadString(hInst, IDS_DELETE, str, 256);
                    InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, IDM_BS_DEL, str);
                    if (iPos > 0)
                    {
                        LoadString(hInst, IDS_MOVE_UP, str, 256);
                        InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, IDM_BS_MOVE_UP, str);
                    }
                    if (iPos < _header->book_source_count-1)
                    {
                        LoadString(hInst, IDS_MOVE_DOWN, str, 256);
                        InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, IDM_BS_MOVE_DOWN, str);
                    }
                    LoadString(hInst, IDS_CLEAR, str, 256);
                    InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, IDM_BS_CLEAR, str);
                    int ret = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hList, NULL);
                    DestroyMenu(hMenu);
                    if (IDM_BS_DEL == ret)
                    {
                        if (IDYES == MessageBox_(hDlg, IDS_DELETE_BS_CFM, IDS_WARN, MB_ICONINFORMATION | MB_YESNO))
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
                            _load_ui(hDlg, iPos, FALSE);
                        }
                    }
                    else if (IDM_BS_MOVE_UP == ret)
                    {
                        // move up
                        book_source_t* item = (book_source_t*)malloc(sizeof(book_source_t));
                        book_source_t* item_1 = &_header->book_sources[iPos];
                        book_source_t* item_2 = &_header->book_sources[iPos - 1];
                        memcpy(item, item_1, sizeof(book_source_t));
                        memcpy(item_1, item_2, sizeof(book_source_t));
                        memcpy(item_2, item, sizeof(book_source_t));
                        free(item);

                        // update ui
                        ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_BOOKSRC));
                        _load_ui(hDlg, iPos - 1, TRUE);
                    }
                    else if (IDM_BS_MOVE_DOWN == ret)
                    {
                        // move down
                        book_source_t* item = (book_source_t*)malloc(sizeof(book_source_t));
                        book_source_t* item_1 = &_header->book_sources[iPos];
                        book_source_t* item_2 = &_header->book_sources[iPos + 1];
                        memcpy(item, item_1, sizeof(book_source_t));
                        memcpy(item_1, item_2, sizeof(book_source_t));
                        memcpy(item_2, item, sizeof(book_source_t));
                        free(item);

                        // update ui
                        ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_BOOKSRC));
                        _load_ui(hDlg, iPos + 1, TRUE);
                    }
                    else if (IDM_BS_CLEAR == ret)
                    {
                        if (IDYES == MessageBox_(hDlg, IDS_CLEAR_BS_CFM, IDS_WARN, MB_ICONINFORMATION | MB_YESNO))
                        {
                            _header->book_source_count = 0;
                            memset(_header->book_sources, 0, sizeof(_header->book_sources));
                            
                            // update ui
                            ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_BOOKSRC));
                            _load_ui(hDlg, -1, TRUE);
                        }
                    }
                }
            }
            else
            {
                ListView_SetItemState(hList, s_iLastPos, LVIS_SELECTED, LVIS_SELECTED);
                return (INT_PTR)FALSE;
            }
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

#endif