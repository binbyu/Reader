#include "stdafx.h"
#include "OnlineDlg.h"
#include "resource.h"
#include "HtmlParser.h"
#include "HttpClient.h"
#include "Utils.h"

static HINSTANCE g_Inst = NULL;
static HWND g_hWnd = NULL;
static BOOL g_Enable = TRUE;
static req_handler_t g_hRequest = NULL;
static std::vector<std::string> g_table_url;
static int g_book_src_idx = -1;
extern header_t* _header;

extern void OnOpenOlBook(HWND, void*);
extern int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType);
int MessageBoxFmt_(HWND hWnd, UINT captionId, UINT uType, UINT formatId, ...);

static INT_PTR CALLBACK OnlineDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK OnBookSourceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static unsigned int writer_buffer(void* data, unsigned int size, unsigned int nmemb, void* stream);
static int OnRequestQuery(char* keyword, HWND hDlg);
static void EnableDialog(HWND hDlg, BOOL enable);

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

BOOL Redirect(const char* host, char* html, int len, HWND hWnd);

static INT_PTR CALLBACK OnlineDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char* keyword = NULL;
    TCHAR text[256] = { 0 };
    char Url[1024] = { 0 };
    TCHAR* path;
    HWND hList;
    int iPos;
    int i;
    switch (message)
    {
    case WM_INITDIALOG:
        g_hRequest = NULL;
        g_Enable = TRUE;
        g_table_url.clear();
        ListView_SetExtendedListViewStyleEx(GetDlgItem(hDlg, IDC_LIST_QUERY), LVS_REPORT|LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_AUTOSIZECOLUMNS|LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
        for (i = 0; i < _header->book_source_count; i++)
        {
            SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_ADDSTRING, 0, (LPARAM)_header->book_sources[i].title);
        }
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_SETCURSEL, 0, NULL);
        EnableDialog(hDlg, TRUE);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            hList = GetDlgItem(hDlg, IDC_LIST_QUERY);
            iPos = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (iPos != -1)
            {
                ol_book_param_t param = { 0 };
                path = Utils::Utf8ToUtf16(g_table_url[iPos].c_str());
                if (_strnicmp("http", g_table_url[iPos].c_str(), 4) != 0)
                {
                    sprintf(Url, "%s%s", _header->book_sources[g_book_src_idx].host, g_table_url[iPos].c_str());
                }
                ListView_GetItemText(hList, iPos, 0, param.book_name, 256);
                ListView_GetItemText(hList, iPos, 5, text, 256);
                if (_tcscmp(text, _T("Íê±¾")) == 0)
                    param.is_finished = 1;
                strcpy(param.main_page, Url);
                strcpy(param.host, _header->book_sources[g_book_src_idx].host);
                OnOpenOlBook(g_hWnd, &param);
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
        case IDCANCEL:
            if (g_hRequest)
            {
                HttpClient::Instance()->Cancel(g_hRequest);
            }
            g_hRequest = NULL;
            g_Enable = TRUE;
            g_table_url.clear();
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_BUTTON_BS_MANAGER:
            OpenBookSourceDlg(g_Inst, hDlg);
            break;
        case IDC_BUTTON_OL_QUERY:
            if (g_Enable)
            {
                GetDlgItemText(hDlg, IDC_EDIT_QUERY_KEYWORD, text, 256);
                if (_tcslen(text) == 0)
                {
                    MessageBox_(hDlg, IDS_EMPTY_KEYWORD, IDS_ERROR, MB_ICONERROR | MB_OK);
                    break;
                }
                EnableDialog(hDlg, FALSE);
                keyword = Utils::Utf16ToUtf8(text);
                ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_QUERY));
                g_table_url.clear();
                OnRequestQuery(keyword, hDlg);
            }
            else
            {
                if (g_hRequest)
                {
                    HttpClient::Instance()->Cancel(g_hRequest);
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

static INT_PTR CALLBACK OnBookSourceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i;
    HWND hList;
    LV_COLUMN lvc = { 0 };
    LVITEM lvitem = { 0 };
    int iPos;
    int count;
    TCHAR buf[1024];
    TCHAR szTitle[256];
    switch (message)
    {
    case WM_INITDIALOG:
        hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);
        ListView_SetExtendedListViewStyleEx(hList, LVS_REPORT | LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
        if (hList)
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
        if (_header->book_source_count > 0)
        {
            SetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, _header->book_sources[0].title);
            SetDlgItemText(hDlg, IDC_EDIT_BS_HOST, Utils::Utf8ToUtf16(_header->book_sources[0].host));
            SetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, Utils::Utf8ToUtf16(_header->book_sources[0].query_url_format));
            SetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TH, Utils::Utf8ToUtf16(_header->book_sources[0].books_th_xpath));
            SetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TD, Utils::Utf8ToUtf16(_header->book_sources[0].books_td_xpath));
            SetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_URL, Utils::Utf8ToUtf16(_header->book_sources[0].book_mainpage_xpath));
            SetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, Utils::Utf8ToUtf16(_header->book_sources[0].chapter_title_xpath));
            SetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, Utils::Utf8ToUtf16(_header->book_sources[0].chapter_url_xpath));
            SetDlgItemText(hDlg, IDC_EDIT_BS_CTX, Utils::Utf8ToUtf16(_header->book_sources[0].content_xpath));
            ListView_SetItemState(hList, 0, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
        }
        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BS_SAVE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            // add item
            hList = GetDlgItem(hDlg, IDC_LIST_BOOKSRC);
            GetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, buf, 256);
            // check is exist
            for (i = 0; i < _header->book_source_count; i++)
            {
                if (0 == _tcscmp(buf, _header->book_sources[i].title))
                {
                    MessageBox_(hDlg, IDS_BOOKSOURCE_EXIST, IDS_ERROR, MB_ICONERROR | MB_OK);
                    return (INT_PTR)FALSE;
                }
            }

            // check max count
            count = ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_BOOKSRC));
            if (count >= MAX_BOOKSRC_COUNT)
            {
                MessageBoxFmt_(hDlg, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_BOOKSOURCE_FULL, MAX_BOOKSRC_COUNT);
                return (INT_PTR)FALSE;
            }

            // check data
            GetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_HOST, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TH, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TD, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_URL, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_CTX, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }

            // update data
            GetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, _header->book_sources[_header->book_source_count].title, 256);
            GetDlgItemText(hDlg, IDC_EDIT_BS_HOST, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].host, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].query_url_format, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TH, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].books_th_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TD, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].books_td_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_URL, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].book_mainpage_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].chapter_title_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].chapter_url_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CTX, buf, 1024);
            strcpy(_header->book_sources[_header->book_source_count].content_xpath, Utils::Utf16ToUtf8(buf));
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
            break;
        case IDCANCEL:
            // update parent combo
            SendMessage(GetDlgItem(GetParent(hDlg), IDC_COMBO_BS_LIST), CB_RESETCONTENT, 0, NULL);
            for (i = 0; i < _header->book_source_count; i++)
            {
                SendMessage(GetDlgItem(GetParent(hDlg), IDC_COMBO_BS_LIST), CB_ADDSTRING, 0, (LPARAM)_header->book_sources[i].title);
            }
            SendMessage(GetDlgItem(GetParent(hDlg), IDC_COMBO_BS_LIST), CB_SETCURSEL, 0, NULL);
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

            // check is exist
            GetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, buf, 256);
            for (i = 0; i < _header->book_source_count; i++)
            {
                if (i != iPos && 0 == _tcscmp(buf, _header->book_sources[i].title))
                {
                    MessageBox_(hDlg, IDS_BOOKSOURCE_EXIST, IDS_ERROR, MB_ICONERROR | MB_OK);
                    return (INT_PTR)FALSE;
                }
            }

            // check data
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_HOST, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TH, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TD, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_URL, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BS_CTX, buf, 256);
            if (!buf[0])
            {
                MessageBox_(hDlg, IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
                return (INT_PTR)FALSE;
            }

            // save
            GetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, _header->book_sources[iPos].title, 256);
            GetDlgItemText(hDlg, IDC_EDIT_BS_HOST, buf, 1024);
            strcpy(_header->book_sources[iPos].host, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, buf, 1024);
            strcpy(_header->book_sources[iPos].query_url_format, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TH, buf, 1024);
            strcpy(_header->book_sources[iPos].books_th_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TD, buf, 1024);
            strcpy(_header->book_sources[iPos].books_td_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_URL, buf, 1024);
            strcpy(_header->book_sources[iPos].book_mainpage_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, buf, 1024);
            strcpy(_header->book_sources[iPos].chapter_title_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, buf, 1024);
            strcpy(_header->book_sources[iPos].chapter_url_xpath, Utils::Utf16ToUtf8(buf));
            GetDlgItemText(hDlg, IDC_EDIT_BS_CTX, buf, 1024);
            strcpy(_header->book_sources[iPos].content_xpath, Utils::Utf16ToUtf8(buf));

            // update list view
            memset(&lvitem, 0, sizeof(LVITEM));
            lvitem.mask = LVIF_TEXT;
            lvitem.cchTextMax = MAX_PATH;
            lvitem.iItem = iPos;
            lvitem.iSubItem = 0;
            lvitem.pszText = _header->book_sources[iPos].title;
            ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);
            break;
        default:
            break;
        }
        break;
    case WM_NOTIFY:
        if (LOWORD(wParam) == IDC_LIST_BOOKSRC)
        {
            if (((LPNMHDR)lParam)->code == NM_DBLCLK)
            {
                iPos = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_BOOKSRC), -1, LVNI_SELECTED);
                if (iPos != -1)
                {
                    SetDlgItemText(hDlg, IDC_EDIT_BS_TITLE, _header->book_sources[iPos].title);
                    SetDlgItemText(hDlg, IDC_EDIT_BS_HOST, Utils::Utf8ToUtf16(_header->book_sources[iPos].host));
                    SetDlgItemText(hDlg, IDC_EDIT_BS_QUERY, Utils::Utf8ToUtf16(_header->book_sources[iPos].query_url_format));
                    SetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TH, Utils::Utf8ToUtf16(_header->book_sources[iPos].books_th_xpath));
                    SetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_TD, Utils::Utf8ToUtf16(_header->book_sources[iPos].books_td_xpath));
                    SetDlgItemText(hDlg, IDC_EDIT_BS_BOOK_URL, Utils::Utf8ToUtf16(_header->book_sources[iPos].book_mainpage_xpath));
                    SetDlgItemText(hDlg, IDC_EDIT_BS_CPT_TITLE, Utils::Utf8ToUtf16(_header->book_sources[iPos].chapter_title_xpath));
                    SetDlgItemText(hDlg, IDC_EDIT_BS_CPT_URL, Utils::Utf8ToUtf16(_header->book_sources[iPos].chapter_url_xpath));
                    SetDlgItemText(hDlg, IDC_EDIT_BS_CTX, Utils::Utf8ToUtf16(_header->book_sources[iPos].content_xpath));
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

static unsigned int writer_buffer(void* data, unsigned int size, unsigned int nmemb, void* stream)
{
    const int SIZE = 10 * 1024; // 10KB
    int len = 0;
    writer_buffer_param_t* param = (writer_buffer_param_t*)stream;

    len = size * nmemb;
    len = (len + SIZE - 1) / SIZE * SIZE;

    if (!param->buf)
    {
        param->total = len;
        param->buf = (unsigned char*)malloc(param->total);
    }
    else
    {
        if (param->total - param->used - 1 < size * nmemb)
        {
            param->total += len;
            param->buf = (unsigned char*)realloc(param->buf, param->total);
        }
    }
    memcpy(param->buf + param->used, data, size * nmemb);
    param->used += size * nmemb;
    param->buf[param->used] = 0;
    return size * nmemb;
}

static unsigned int RequestQueryCompleter(bool result, request_t* req, int isgzip)
{
    writer_buffer_param_t* writer = (writer_buffer_param_t*)req->stream;
    char* html = NULL;
    int htmllen = 0;
    HWND hDlg = (HWND)req->param1;
    std::vector<std::string> table_th;
    std::vector<std::string> table_td;
    HWND hList = NULL;
    LV_COLUMN lvc = { 0 };
    LVITEM lvitem = { 0 };
    int i, j;
    void* doc = NULL;
    void* ctx = NULL;
    bool cancel = false;

    g_hRequest = NULL;

    if (req->cancel)
    {
        free(writer->buf);
        free(writer);
        EnableDialog(hDlg, TRUE);
        return 1;
    }

    if (!result)
    {
        free(writer->buf);
        free(writer);
        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_NETWORK_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    if (isgzip)
    {
        if (!Utils::gzipInflate(writer->buf, writer->used, (unsigned char**)&html, &htmllen))
        {
            free(writer->buf);
            free(writer);
            EnableDialog(hDlg, TRUE);
            MessageBox_(hDlg, IDS_DECOMPRESS_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
            return 1;
        }
        free(writer->buf);
    }
    else
    {
        html = (char*)writer->buf;
        htmllen = writer->used;
    }
    free(writer);

    HtmlParser::Instance()->HtmlParseBegin(html, htmllen, &doc, &ctx, &cancel);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[g_book_src_idx].books_th_xpath, table_th, &cancel);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[g_book_src_idx].books_td_xpath, table_td, &cancel);
    HtmlParser::Instance()->HtmlParseByXpath(doc, ctx, _header->book_sources[g_book_src_idx].book_mainpage_xpath, g_table_url, &cancel);
    HtmlParser::Instance()->HtmlParseEnd(doc, ctx);

    // redirect ... test
    if (/*table_td.size() == 0 || */table_th.size() == 0 /*|| g_table_url.size() == 0*/ || (table_th.size() != 0 && table_td.size() / table_th.size() != g_table_url.size()))
    {
        if (Redirect(_header->book_sources[g_book_src_idx].host, html, htmllen, hDlg))
        {
            free(html);
            return 0;
        }
    }
    free(html);

    // check value
    if (/*table_td.size() == 0 || */table_th.size() == 0 /*|| g_table_url.size() == 0*/ || (table_th.size()!=0 && table_td.size() / table_th.size() != g_table_url.size()))
    {
        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_PARSE_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }

    hList = GetDlgItem(hDlg, IDC_LIST_QUERY);
    if (hList)
    {
        for (i = 0; i < (int)table_th.size(); i++)
        {
            memset(&lvc, 0, sizeof(LV_COLUMN));
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            lvc.pszText = Utils::Utf8ToUtf16(table_th[i].c_str());
            lvc.cx = i==0?120:60;
            SendMessage(hList, LVM_INSERTCOLUMN, i, (LPARAM)&lvc);
        }
        
        for (i = 0; i < (int)(table_td.size() / table_th.size()); i++)
        {
            for (j = 0; j < (int)table_th.size(); j++)
            {
                memset(&lvitem, 0, sizeof(LVITEM));
                lvitem.mask = LVIF_TEXT;
                lvitem.cchTextMax = MAX_PATH;
                lvitem.iItem = i;
                lvitem.iSubItem = j;
                lvitem.pszText = Utils::Utf8ToUtf16(table_td[i * table_th.size() + j].c_str());
                ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
                ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);
            }
        }
    }

    EnableDialog(hDlg, TRUE);
    return 0;
}

static int OnRequestQuery(char* keyword, HWND hDlg)
{
    char* query_format;
    char url[1024];
    char* encode;
    request_t req;
    writer_buffer_param_t* writer = NULL;
    int sel;

    sel = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
    if (sel < 0 || sel >= MAX_BOOKSRC_COUNT)
    {
        EnableDialog(hDlg, TRUE);
        MessageBox_(hDlg, IDS_SELECT_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    g_book_src_idx = sel;
    query_format = _header->book_sources[g_book_src_idx].query_url_format;

    Utils::UrlEncode(keyword, &encode);
    sprintf(url, query_format, encode);
    Utils::UrlFree(encode);

    // parser chapter
    writer = (writer_buffer_param_t*)malloc(sizeof(writer_buffer_param_t));
    memset(writer, 0, sizeof(writer_buffer_param_t));
    
    req.method = GET;
    req.url = url;
    req.writer = writer_buffer;
    req.stream = writer;
    req.completer = RequestQueryCompleter;
    req.param1 = hDlg;
    req.param2 = NULL;

    g_hRequest = HttpClient::Instance()->Request(req);
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

BOOL Redirect(const char* host, char* html, int len, HWND hWnd)
{
    const redirect_kw_t redirect_kw[] =
    {
        {"window.location=\"","\""},
        {"URL: ","<"}
    };

    char url[1024] = { 0 };
    request_t req;
    writer_buffer_param_t* writer = NULL;
    char* b, * e;
    int i;
#if TEST_MODEL
    char msg[1024] = { 0 };
#endif

    if (!html || len <= 0)
        return FALSE;

    for (i = 0; i < sizeof(redirect_kw) / sizeof(redirect_kw_t); i++)
    {
        b = strstr(html, redirect_kw[i].begin);
        if (b)
        {
            b += strlen(redirect_kw[i].begin);
            e = strstr(b, redirect_kw[i].end);
            if (e)
            {
                *e = 0;

                if (strncmp(b, "http", 4) == 0)
                {
                    strcpy(url, b);
                }
                else if (strncmp(b, "www.", 4) == 0)
                {
                    strcpy(url, "http://");
                    strcat(url, b);
                }
                else
                {
                    strcpy(url, host);
                    strcat(url, b);
                }

                writer = (writer_buffer_param_t*)malloc(sizeof(writer_buffer_param_t));
                memset(writer, 0, sizeof(writer_buffer_param_t));
#if TEST_MODEL
                sprintf(msg, "Redirect request to: %s\n", url);
                OutputDebugStringA(msg);
#endif
                req.method = GET;
                req.url = url;
                req.writer = writer_buffer;
                req.stream = writer;
                req.completer = RequestQueryCompleter;
                req.param1 = hWnd;
                req.param2 = NULL;

                g_hRequest = HttpClient::Instance()->Request(req);
                return TRUE;
            }
        }
    }
    return FALSE;
}
