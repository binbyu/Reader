#include "tagset.h"

#if ENABLE_TAG
#include "resource.h"
#include <CommDlg.h>

extern header_t *_header;
extern HWND _hWnd;
extern HINSTANCE hInst;
extern void Save(HWND);
extern LRESULT CALLBACK ChooseFontComboProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR udata);
extern int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType);

typedef int (*tag_item_callback_t)(tagitem_t *tag, int idx);
static void TS_item_OpenDlg(tag_item_callback_t cb, tagitem_t *tags, int idx);
static INT_PTR CALLBACK TS_item_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static UINT_PTR CALLBACK ChooseFontProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK TS_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

static int _add_tag_item(tagitem_t *tag, int idx)
{
    idx = _header->tag_count;

    if (idx >= MAX_TAG_COUNT)
        return 1;

    _header->tag_count ++;
    tag->enable = TRUE;
    memcpy(&_header->tags[idx], tag, sizeof(tagitem_t));
    Save(_hWnd);
    return 0;
}

static int _edit_tag_item(tagitem_t *tag, int idx)
{
    if (idx >= MAX_TAG_COUNT || idx >= _header->tag_count)
        return 1;

    tag->enable = TRUE; // for no checkbox
    if (memcmp(&_header->tags[idx], tag, sizeof(tagitem_t)) != 0)
    {
        memcpy(&_header->tags[idx], tag, sizeof(tagitem_t));
        Save(_hWnd);
    }
    return 0;
}

static int _delete_tag_item(tagitem_t *tag, int idx)
{
    int count;

    if (_header->tag_count <= 0)
        return 1;

    if (idx >= MAX_TAG_COUNT || idx >= _header->tag_count)
        return 1;

    _header->tag_count --;
    count = _header->tag_count - idx;
    if (count > 0)
    {
        memcpy(&_header->tags[idx], &_header->tags[idx+1], sizeof(tagitem_t)*count);
    }
    memset(&_header->tags[_header->tag_count], 0, sizeof(tagitem_t));
    Save(_hWnd);
    return 0;
}

static int _enable_tag_item(tagitem_t *tag, int idx)
{
    if (idx >= MAX_TAG_COUNT || idx >= _header->tag_count)
        return 1;

    if (!_header->tags[idx].enable)
    {
        _header->tags[idx].enable = TRUE;
        Save(_hWnd);
    }
    return 0;
}

static int _disable_tag_item(tagitem_t *tag, int idx)
{
    if (idx >= MAX_TAG_COUNT || idx >= _header->tag_count)
        return 1;

    if (_header->tags[idx].enable)
    {
        _header->tags[idx].enable = FALSE;
        Save(_hWnd);
    }
    return 0;
}

static int _exist_tag_item(tagitem_t* tag, int idx)
{
    int i;
    for (i=0; i<_header->tag_count; i++)
    {
        if (_tcscmp(tag->keyword, _header->tags[i].keyword) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static int _full_tagset(tagitem_t* tag, int idx)
{
    if (_header->tag_count >= MAX_TAG_COUNT)
    {
        return 1;
    }
    return 0;
}

static int _rm_all_tagset(tagitem_t* tag, int idx)
{
    _header->tag_count = 0;
    memset(&_header->tags, 0, sizeof(_header->tags));
    return 0;
}

static void _update_tagset_list(HWND hDlg, int idx)
{
    HWND hList = NULL;
    HWND hHeader = NULL;
    LV_COLUMN lvc = { 0 };
    LVITEM lvitem = { 0 };
    TCHAR colname[256] = { 0 };
    int colnum = 0;
    int i,col;

    hList = GetDlgItem(hDlg, IDC_LIST_TAGSET);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    colnum = SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0);

    // add list header
    if (colnum == 0)
    {
        // ID
        LoadString(hInst, IDS_ID, colname, 256);
        memset(&lvc, 0, sizeof(LV_COLUMN));
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.pszText = colname;
        lvc.cx = 60;
        SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);

        // keyword
        LoadString(hInst, IDS_KEYWORD, colname, 256);
        memset(&lvc, 0, sizeof(LV_COLUMN));
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.pszText = colname;
        lvc.cx = 180;
        SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc);

        // font
        LoadString(hInst, IDS_FONT, colname, 256);
        memset(&lvc, 0, sizeof(LV_COLUMN));
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.pszText = colname;
        lvc.cx = 100;
        SendMessage(hList, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc);

        // bg color
        LoadString(hInst, IDS_BG_COLOR, colname, 256);
        memset(&lvc, 0, sizeof(LV_COLUMN));
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.pszText = colname;
        lvc.cx = 80;
        SendMessage(hList, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc);
    }

    ListView_DeleteAllItems(hList);
    for (i=0; i<_header->tag_count; i++)
    {
        col = 0;

        // ID
        _stprintf(colname, _T("%d"), i+1);
        memset(&lvitem, 0, sizeof(LVITEM));
        lvitem.mask = LVIF_TEXT;
        lvitem.cchTextMax = MAX_PATH;
        lvitem.iItem = i;
        lvitem.iSubItem = col++;
        lvitem.pszText = colname;
        //lvitem.stateMask = LVIS_STATEIMAGEMASK;
        //lvitem.state = _header->tags[i].enable ? 0x2000 : 0x1000;
        ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
        ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);

        // keyword
        memset(&lvitem, 0, sizeof(LVITEM));
        lvitem.mask = LVIF_TEXT;
        lvitem.cchTextMax = MAX_PATH;
        lvitem.iItem = i;
        lvitem.iSubItem = col++;
        lvitem.pszText = _header->tags[i].keyword;
        ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
        ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);

        // font
        memset(&lvitem, 0, sizeof(LVITEM));
        lvitem.mask = LVIF_TEXT;
        lvitem.cchTextMax = MAX_PATH;
        lvitem.iItem = i;
        lvitem.iSubItem = col++;
        lvitem.pszText = _header->tags[i].font.lfFaceName;
        ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
        ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);

        // bg color
        wsprintf(colname, _T("0x%08X"), _header->tags[i].bg_color);
        memset(&lvitem, 0, sizeof(LVITEM));
        lvitem.mask = LVIF_TEXT;
        lvitem.cchTextMax = MAX_PATH;
        lvitem.iItem = i;
        lvitem.iSubItem = col++;
        lvitem.pszText = colname;
        ::SendMessage(hList, LVM_INSERTITEM, lvitem.iItem, (LPARAM)&lvitem);
        ::SendMessage(hList, LVM_SETITEMTEXT, lvitem.iItem, (LPARAM)&lvitem);

        //ListView_SetCheckState(hList, i, _header->tags[i].enable);
    }

    ListView_SetItemState(hList, idx, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
}

static void _update_tagset_preview(HWND hDlg)
{
    HWND hStatic = NULL;
    HWND hList = NULL;
    int iPos;
    static HFONT s_font = NULL;

    if (s_font)
    {
        DeleteObject(s_font);
        s_font = NULL;
    }

    hList = GetDlgItem(hDlg, IDC_LIST_TAGSET);
    hStatic = GetDlgItem(hDlg, IDC_STATIC_PREVIEW);

    iPos = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (iPos >= 0 && iPos < _header->tag_count)
    {
        s_font = CreateFontIndirect(&_header->tags[iPos].font);
        SendMessage(hStatic, WM_SETFONT, (WPARAM)s_font, NULL);
        SetDlgItemText(hDlg, IDC_STATIC_PREVIEW, _header->tags[iPos].keyword);
    }
    else
    {
        SetDlgItemText(hDlg, IDC_STATIC_PREVIEW, _T(""));
    }
}

void TS_OpenDlg(void)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_TAGSET), _hWnd, TS_DlgProc);
}

INT_PTR CALLBACK TS_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HFONT s_font = NULL;
    HWND hList = NULL;
    int iPos;
    TCHAR str[256] = { 0 };
    static int s_iLastPos = -1;
    LPNMLISTVIEW pitem;

    switch (message)
    {
    case WM_INITDIALOG:
        hList = GetDlgItem(hDlg, IDC_LIST_TAGSET);
        ListView_SetExtendedListViewStyleEx(hList, LVS_REPORT|LVM_SETEXTENDEDLISTVIEWSTYLE, 
            LVS_EX_AUTOSIZECOLUMNS|LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT/*|LVS_EX_CHECKBOXES*/);
        SetDlgItemText(hDlg, IDC_STATIC_PREVIEW, _T(""));
        _update_tagset_list(hDlg, 0);
        _update_tagset_preview(hDlg);
        s_iLastPos = 0;
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (s_font)
            {
                DeleteObject(s_font);
                s_font = NULL;
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            if (s_font)
            {
                DeleteObject(s_font);
                s_font = NULL;
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_BUTTON_ADD:
            if (_full_tagset(NULL, -1))
            {
                MessageBox_(hDlg, IDS_TAGSET_FULL, IDS_ERROR, MB_ICONERROR | MB_OK);
                break;
            }
            iPos = _header->tag_count;
            TS_item_OpenDlg(_add_tag_item, &_header->tags[_header->tag_count], _header->tag_count);
            if (_header->tag_count > iPos)
            {
                s_iLastPos = _header->tag_count - 1;
                _update_tagset_list(hDlg, s_iLastPos);
            }
            _update_tagset_preview(hDlg);
            break;
        case IDC_BUTTON_RMALL:
            if (IDYES == MessageBox_(hDlg, IDS_RMALL_CFM, IDS_WARN, MB_ICONINFORMATION | MB_YESNO))
            {
                // delete from data
                _rm_all_tagset(NULL, -1);
                _update_tagset_list(hDlg, s_iLastPos);
                _update_tagset_preview(hDlg);
            }
            break;
        default:
            break;
        }
        break;
    case WM_NOTIFY:
        switch (LOWORD(wParam))
        {
        case IDC_LIST_TAGSET:
            switch (((LPNMHDR)lParam)->code)
            {
#if 1
            case NM_CLICK:
            case NM_DBLCLK:
                hList = GetDlgItem(hDlg, IDC_LIST_TAGSET);
                iPos = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                if (iPos != -1)
                {
                    if (iPos != s_iLastPos)
                    {
                        s_iLastPos = iPos;
                        _update_tagset_preview(hDlg);
                    }
                }
                else
                {
                    if (iPos != s_iLastPos)
                    {
                        s_iLastPos = iPos;
                        _update_tagset_preview(hDlg);
                    }
                }
                break;
#endif
            case LVN_ITEMCHANGED:
                pitem = ((LPNMLISTVIEW)lParam);
                iPos = pitem->iItem;
                if (LVIS_SELECTED & pitem->uNewState)
                {
                    if (iPos != s_iLastPos)
                    {
                        s_iLastPos = iPos;
                        _update_tagset_preview(hDlg);
                    }
                }
#if 0
                else if (0x2000 == (pitem->uNewState & LVIS_STATEIMAGEMASK)) // Item checked
                {
                    _enable_tag_item(&_header->tags[iPos], iPos);
                }
                else if (0x1000 == (pitem->uNewState & LVIS_STATEIMAGEMASK)) // Item unchecked
                {
                    _disable_tag_item(&_header->tags[iPos], iPos);
                }
#endif
                break;
            }
            break;
        default:
            break;
        }
        break;
    case WM_CONTEXTMENU:
        hList = GetDlgItem(hDlg, IDC_LIST_TAGSET);
        if (wParam == (WPARAM)hList)
        {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            iPos = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_TAGSET), -1, LVNI_SELECTED);
            if (iPos >= 0 && iPos < _header->tag_count)
            {
                s_iLastPos = iPos;
                if (iPos != s_iLastPos)
                {
                    s_iLastPos = iPos;
                    _update_tagset_preview(hDlg);
                }
                HMENU hMenu = CreatePopupMenu();
                if (hMenu)
                {
                    LoadString(hInst, IDS_DELETE, str, 256);
                    InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_BS_DEL, str);
                    LoadString(hInst, IDS_EDIT, str, 256);
                    InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_TS_EDIT, str);
                    int ret = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hList, NULL);
                    DestroyMenu(hMenu);
                    if (IDM_BS_DEL == ret)
                    {
                        if (IDYES == MessageBox_(hDlg, IDS_DELETE_CFM, IDS_WARN, MB_ICONINFORMATION | MB_YESNO))
                        {
                            // delete from data
                            _delete_tag_item(NULL, iPos);

                            // delete  list view
                            ListView_DeleteItem(hList, iPos);

                            _update_tagset_preview(hDlg);
                        }
                    }
                    else if (IDM_TS_EDIT == ret)
                    {
                        // edit data
                        TS_item_OpenDlg(_edit_tag_item, &_header->tags[iPos], iPos);

                        // update ui
                        _update_tagset_list(hDlg, iPos);

                        _update_tagset_preview(hDlg);
                    }
                }
            }
            else
            {
                ListView_SetItemState(hList, s_iLastPos, LVIS_SELECTED, LVIS_SELECTED);
                _update_tagset_preview(hDlg);
                return (INT_PTR)FALSE;
            }
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            if (IDC_STATIC_PREVIEW == GetDlgCtrlID((HWND)lParam))
            {
                hList = GetDlgItem(hDlg, IDC_LIST_TAGSET);
                iPos = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                if (iPos >= 0 && iPos < _header->tag_count)
                {
                    SetBkColor((HDC)wParam, _header->tags[iPos].bg_color);
                    SetBkMode((HDC)wParam, OPAQUE);
                    SetTextColor((HDC)wParam, _header->tags[iPos].font_color);
                }
            }
            else
            {
                SetBkMode((HDC)wParam, TRANSPARENT);
            }
            return (BOOL)CreateSolidBrush (GetSysColor(COLOR_MENU));
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

//////////////////////////////////////////////////////////////////////
/// tag item dialog
tag_item_callback_t g_tag_item_cb = NULL;
static tagitem_t g_tag_item = {0};
static int g_tag_item_idx = NULL;
void TS_item_OpenDlg(tag_item_callback_t cb, tagitem_t *tags, int idx)
{
    g_tag_item_cb = cb;
    if (tags)
        memcpy(&g_tag_item, tags, sizeof(tagitem_t));
    else
        memset(&g_tag_item, 0, sizeof(tagitem_t));
    g_tag_item_idx = idx;
    DialogBox(hInst, MAKEINTRESOURCE(IDD_TAGITEM), _hWnd, TS_item_DlgProc);
}

INT_PTR CALLBACK TS_item_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HFONT s_font = NULL;

    switch (message)
    {
    case WM_INITDIALOG:
        if (s_font)
        {
            DeleteObject(s_font);
        }
        s_font = CreateFontIndirect(&g_tag_item.font);
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_TAGKW), g_tag_item.keyword);
        SetWindowText(GetDlgItem(hDlg, IDC_STATIC_PREVIEW), g_tag_item.keyword);
        SendMessage(GetDlgItem(hDlg, IDC_STATIC_PREVIEW), WM_SETFONT, (WPARAM)s_font, NULL);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (_add_tag_item == g_tag_item_cb)
            {
                if (_exist_tag_item(&g_tag_item, g_tag_item_idx))
                {
                    MessageBox_(hDlg, IDS_TAGITEM_EXIST, IDS_ERROR, MB_ICONERROR | MB_OK);
                    break;
                }
            }

            if (g_tag_item.keyword[0] != 0)
            {
                if (g_tag_item_cb)
                {
                    g_tag_item_cb(&g_tag_item, g_tag_item_idx);
                }
            }

            if (s_font)
            {
                DeleteObject(s_font);
                s_font = NULL;
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            if (s_font)
            {
                DeleteObject(s_font);
                s_font = NULL;
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_BUTTON_TAGFONT:
            {
                CHOOSEFONT cf;            // common dialog box structure
                LOGFONT logFont;
                static DWORD rgbCurrent;   // current text color
                BOOL bUpdate = FALSE;

                // Initialize CHOOSEFONT
                ZeroMemory(&cf, sizeof(cf));
                memcpy(&logFont, &(g_tag_item.font), sizeof(LOGFONT));
                cf.lStructSize = sizeof (cf);
                cf.hwndOwner = hDlg;
                cf.lpLogFont = &logFont;
                cf.rgbColors = g_tag_item.font_color;
                cf.lpfnHook = ChooseFontProc;
                cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS | CF_NOVERTFONTS | CF_ENABLEHOOK;

                if (ChooseFont(&cf))
                {
                    if (g_tag_item.font_color != cf.rgbColors)
                    {
                        g_tag_item.font_color = cf.rgbColors;
                        bUpdate = TRUE;
                    }
                    g_tag_item.font.lfQuality = PROOF_QUALITY;

                    if (0 != memcmp(&logFont, &g_tag_item.font, sizeof(LOGFONT)))
                    {
                        memcpy(&g_tag_item.font, &logFont, sizeof(LOGFONT));
                        bUpdate = TRUE;

                        if (s_font)
                        {
                            DeleteObject(s_font);
                        }
                        s_font = CreateFontIndirect(&g_tag_item.font);
                        SendMessage(GetDlgItem(hDlg, IDC_STATIC_PREVIEW), WM_SETFONT, (WPARAM)s_font, NULL);
                    }
                    if (bUpdate)
                    {
                        InvalidateRect(hDlg, NULL, TRUE);
                    }
                }
            }
            break;
        case IDC_BUTTON_TAGBG:
            {
                CHOOSECOLOR cc;                 // common dialog box structure 

                ZeroMemory(&cc, sizeof(cc));
                cc.lStructSize = sizeof(cc);
                cc.hwndOwner = hDlg;
                cc.lpCustColors = (LPDWORD)_header->cust_colors;
                cc.rgbResult = g_tag_item.bg_color;
                cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                if (ChooseColor(&cc))
                {
                    if (g_tag_item.bg_color != cc.rgbResult)
                    {
                        g_tag_item.bg_color = cc.rgbResult;
                        InvalidateRect(hDlg, NULL, TRUE);
                    }
                }
            }
            break;
        case IDC_EDIT_TAGKW:
            GetDlgItemText(hDlg, IDC_EDIT_TAGKW, g_tag_item.keyword, 63);
            SetDlgItemText(hDlg, IDC_STATIC_PREVIEW, g_tag_item.keyword);
            break;
        default:
            break;
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            if (IDC_STATIC_PREVIEW == GetDlgCtrlID((HWND)lParam))
            {
                SetBkColor((HDC)wParam, g_tag_item.bg_color);
                SetBkMode((HDC)wParam, OPAQUE);
                SetTextColor((HDC)wParam, g_tag_item.font_color);
            }
            else
            {
                SetBkMode((HDC)wParam, TRANSPARENT);
            }
            return (BOOL)CreateSolidBrush (GetSysColor(COLOR_MENU));
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

static UINT_PTR CALLBACK ChooseFontProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hComb;
    switch (message)
    {
    case WM_INITDIALOG:
        hComb = GetDlgItem(hWnd, 0x473);
        SendMessage(hComb, CB_SETITEMDATA, 0, _header->font_color);
        SendMessage(hComb, CB_SETCURSEL, 0, _header->font_color);
        SetWindowSubclass(hComb, ChooseFontComboProc, 0, 0);
        break;
    }
    return 0;
};

#endif
