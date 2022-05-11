#include "Keyset.h"
#include "resource.h"

extern LRESULT OnHideWin(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnHideBorder(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnFullScreen(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnTopmost(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnOpenFile(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnAddMark(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnAutoPage(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnSearch(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnJump(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnEditMode(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnPageUp(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnPageDown(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnLineUp(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnLineDown(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnChapterUp(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnChapterDown(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnFontZoomIn(HWND, UINT, WPARAM, LPARAM);
extern LRESULT OnFontZoomOut(HWND, UINT, WPARAM, LPARAM);
extern int MessageBoxFmt_(HWND hWnd, UINT captionId, UINT uType, UINT formatId, ...);

extern HINSTANCE hInst;
extern header_t* _header;
extern HWND _hWnd;
extern void Save(HWND);
extern void SetGlobalKey(HWND);

static BOOL _register_hotkey(HWND hWnd, DWORD kid, DWORD val, const TCHAR *dest);
static BOOL _unregister_hotkey(HWND hWnd, DWORD kid);

#define IDHK_SHWIN ID_HOTKEY_SHOW_HIDE_WINDOW
keydata_t g_Keysets[KI_MAXCOUNT] = 
{
    { MAKELONG(KT_HOTKEY, KI_HIDE),             IDHK_SHWIN, 0, 0, MAKEWORD('H', HOTKEYF_ALT),                      IDC_HK_HIDE,        IDC_CHECK_HIDE,        OnHideWin,     IDS_HIDE_SHOW_WINDOW },
    { MAKELONG(KT_SHORTCUTKEY, KI_BORDER),      0,          0, 0, MAKEWORD(VK_F12, 0),                             IDC_HK_BORDER,      IDC_CHECK_BORDER,      OnHideBorder,  IDS_HIDE_SHOW_BORDER },
    { MAKELONG(KT_SHORTCUTKEY, KI_FULLSCREEN),  0,          0, 0, MAKEWORD(VK_F11, 0),                             IDC_HK_FULLSCREEN,  IDC_CHECK_FULLSCREEN,  OnFullScreen,  IDS_FULLSRCEEN },
    { MAKELONG(KT_SHORTCUTKEY, KI_TOP),         0,          0, 0, MAKEWORD('T', HOTKEYF_CONTROL),                  IDC_HK_TOP,         IDC_CHECK_TOP,         OnTopmost,     IDS_TOPMOST },
    { MAKELONG(KT_SHORTCUTKEY, KI_OPEN),        0,          0, 0, MAKEWORD('O', HOTKEYF_CONTROL),                  IDC_HK_OPEN,        IDC_CHECK_OPEN,        OnOpenFile,    IDS_OPEN_FILE },
    { MAKELONG(KT_SHORTCUTKEY, KI_ADDMARK),     0,          0, 0, MAKEWORD('M', HOTKEYF_CONTROL),                  IDC_HK_ADDMARK,     IDC_CHECK_ADDMARK,     OnAddMark,     IDS_ADD_BOOKMARK },
    { MAKELONG(KT_SHORTCUTKEY, KI_AUTOPAGE),    0,          0, 0, MAKEWORD(VK_SPACE, 0),                           IDC_HK_AUTOPAGE,    IDC_CHECK_AUTOPAGE,    OnAutoPage,    IDS_AUTO_PAGE },
    { MAKELONG(KT_SHORTCUTKEY, KI_SEARCH),      0,          0, 0, MAKEWORD('F', HOTKEYF_CONTROL),                  IDC_HK_SEARCH,      IDC_CHECK_SEARCH,      OnSearch,      IDS_TEXT_SEARCH },
    { MAKELONG(KT_SHORTCUTKEY, KI_JUMP),        0,          0, 0, MAKEWORD('G', HOTKEYF_CONTROL),                  IDC_HK_JUMP,        IDC_CHECK_JUMP,        OnJump,        IDS_PROGRESS_JUMP },
    { MAKELONG(KT_SHORTCUTKEY, KI_EDIT),        0,          0, 0, MAKEWORD('E', HOTKEYF_CONTROL),                  IDC_HK_EDIT,        IDC_CHECK_EDIT,        OnEditMode,    IDS_EDIT_MODE },
    { MAKELONG(KT_SHORTCUTKEY, KI_PAGEUP),      0,          0, 0, MAKEWORD(VK_LEFT, HOTKEYF_EXT),                  IDC_HK_PAGEUP,      IDC_CHECK_PAGEUP,      OnPageUp,      IDS_PAGE_UP },
    { MAKELONG(KT_SHORTCUTKEY, KI_PAGEDOWN),    0,          0, 0, MAKEWORD(VK_RIGHT, HOTKEYF_EXT),                 IDC_HK_PAGEDOWN,    IDC_CHECK_PAGEDOWN,    OnPageDown,    IDS_PAGE_DOWN },
    { MAKELONG(KT_SHORTCUTKEY, KI_LINEUP),      0,          0, 0, MAKEWORD(VK_UP, HOTKEYF_EXT),                    IDC_HK_LINEUP,      IDC_CHECK_LINEUP,      OnLineUp,      IDS_LINE_UP },
    { MAKELONG(KT_SHORTCUTKEY, KI_LINEDOWN),    0,          0, 0, MAKEWORD(VK_DOWN, HOTKEYF_EXT),                  IDC_HK_LINEDOWN,    IDC_CHECK_LINEDOWN,    OnLineDown,    IDS_LINE_DOWN },
    { MAKELONG(KT_SHORTCUTKEY, KI_CHAPTERUP),   0,          0, 0, MAKEWORD(VK_LEFT, HOTKEYF_EXT|HOTKEYF_CONTROL),  IDC_HK_CHAPTERUP,   IDC_CHECK_CHAPTERUP,   OnChapterUp,   IDS_CHAPTER_UP },
    { MAKELONG(KT_SHORTCUTKEY, KI_CHAPTERDOWN), 0,          0, 0, MAKEWORD(VK_RIGHT, HOTKEYF_EXT|HOTKEYF_CONTROL), IDC_HK_CHAPTERDOWN, IDC_CHECK_CHAPTERDOWN, OnChapterDown, IDS_CHAPTER_DOWN },
    { MAKELONG(KT_SHORTCUTKEY, KI_FONTZOOMIN),  0,          0, 0, MAKEWORD(VK_OEM_PLUS, HOTKEYF_CONTROL),          IDC_HK_FONTZOOMIN,  IDC_CHECK_FONTZOOMIN,  OnFontZoomIn,  IDS_FONT_ZOOMIN },
    { MAKELONG(KT_SHORTCUTKEY, KI_FONTZOOMOUT), 0,          0, 0, MAKEWORD(VK_OEM_MINUS, HOTKEYF_CONTROL),         IDC_HK_FONTZOOMOUT, IDC_CHECK_FONTZOOMOUT, OnFontZoomOut, IDS_FONT_ZOOMOUT }
};


void KS_Init(HWND hWnd, keyset_t *keyset)
{
    KS_UpdateKeyset(keyset);

    // register hot key
    KS_RegisterAllHotKey(hWnd);

    // global key
    SetGlobalKey(hWnd);
}

void KS_UpdateKeyset(keyset_t *keyset)
{
    int i;

    if (!keyset)
        return;

    for (i=KI_HIDE; i<KI_MAXCOUNT && i<MAX_KEYSET_COUNT; i++)
    {
        g_Keysets[i].pvalue =  &(keyset[i].value);
        g_Keysets[i].is_disable = &(keyset[i].is_disable);
    }
}

void KS_GetDefaultKeyset(keyset_t *keyset)
{
    int i;

    if (!keyset)
        return;

    for (i=KI_HIDE; i<KI_MAXCOUNT && i<MAX_KEYSET_COUNT; i++)
    {
        keyset[i].value = g_Keysets[i].defval;
        keyset[i].is_disable = 0;
    }
}

void KS_OpenDlg(void)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_KEYSET), _hWnd, KS_DlgProc);
}

INT_PTR CALLBACK KS_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i,j;
    LRESULT res;
    DWORD tempkeys[KI_MAXCOUNT] = {0};
    int tempable[KI_MAXCOUNT] = {0};
    DWORD ctrl_id = 0;
    TCHAR desc[256] = { 0 };

    switch (message)
    {
    case WM_INITDIALOG:
#if ENABLE_GLOBAL_KEY
        if (_header->global_key)
            SendMessage(GetDlgItem(hDlg, IDC_CHECK), BM_SETCHECK, BST_CHECKED, NULL);
        else
            SendMessage(GetDlgItem(hDlg, IDC_CHECK), BM_SETCHECK, BST_UNCHECKED, NULL);
#else
        ShowWindow(GetDlgItem(hDlg, IDC_CHECK), SW_HIDE);
#endif
        for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
        {
            SendMessage(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), HKM_SETHOTKEY, *(g_Keysets[i].pvalue), 0);
            if (LOWORD(g_Keysets[i].key) == KT_SHORTCUTKEY)
            {
                SendMessage(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), HKM_SETRULES, HKCOMB_A, 0);
            }
            if (*(g_Keysets[i].is_disable))
            {
                SendMessage(GetDlgItem(hDlg, g_Keysets[i].able_id), BM_SETCHECK, BST_UNCHECKED, NULL);
                EnableWindow(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), FALSE);
            }
            else
            {
                SendMessage(GetDlgItem(hDlg, g_Keysets[i].able_id), BM_SETCHECK, BST_CHECKED, NULL);
                EnableWindow(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), TRUE);
            }
        }
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
            {
                tempkeys[i] = (DWORD)SendMessage(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), HKM_GETHOTKEY, 0, 0);
                tempable[i] = SendMessage(GetDlgItem(hDlg, g_Keysets[i].able_id), BM_GETCHECK, 0, NULL) == BST_CHECKED ? 0 : 1;
            }
            // check duplicate
            for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
            {
                for (j=i+1; j<KI_MAXCOUNT; j++)
                {
                    if (tempkeys[i] == 0)
                    {
                        LoadString(hInst, g_Keysets[i].desc, desc, 256);
                        MessageBoxFmt_(hDlg, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_KEYVALUE_EMPTY, desc);
                        return (INT_PTR)FALSE;
                    }
                    if (tempkeys[i] == tempkeys[j])
                    {
                        LoadString(hInst, g_Keysets[i].desc, desc, 256);
                        MessageBoxFmt_(hDlg, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_KEYVALUE_DUPLICATE, desc);
                        return (INT_PTR)FALSE;
                    }
                }
            }
            // check hotkey
            for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
            {
                if (LOWORD(g_Keysets[i].key) == KT_HOTKEY)
                {
                    if (tempable[i])
                    {
                        if (!(*(g_Keysets[i].is_disable)))
                            _unregister_hotkey(GetParent(hDlg), g_Keysets[i].key_id);
                    }
                    else
                    {
                        if (!(*(g_Keysets[i].is_disable)))
                        {
                            if (*(g_Keysets[i].pvalue) != tempkeys[i])
                            {
                                LoadString(hInst, g_Keysets[i].desc, desc, 256);
                                _unregister_hotkey(GetParent(hDlg), g_Keysets[i].key_id);
                                if (!_register_hotkey(GetParent(hDlg), g_Keysets[i].key_id, tempkeys[i], desc))
                                {
                                    _register_hotkey(GetParent(hDlg), g_Keysets[i].key_id, *(g_Keysets[i].pvalue), desc);
                                    return (INT_PTR)FALSE;
                                }
                            }
                        }
                        else
                        {
                            if (!_register_hotkey(GetParent(hDlg), g_Keysets[i].key_id, tempkeys[i], desc))
                            {
                                return (INT_PTR)FALSE;
                            }
                        }
                    }
                }
            }
            // set value
            for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
            {
                *(g_Keysets[i].pvalue) = tempkeys[i];
                *(g_Keysets[i].is_disable) = tempable[i];
            }
#if ENABLE_GLOBAL_KEY
            // set global key
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK), BM_GETCHECK, 0, NULL);
            if (res == BST_CHECKED)
            {
                _header->global_key = 1;
            }
            else
            {
                _header->global_key = 0;
            }
#endif
            Save(_hWnd);
            SetGlobalKey(_hWnd);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_BUTTON_DEFAULT:
            for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
            {
                SendMessage(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), HKM_SETHOTKEY, g_Keysets[i].defval, 0);
                SendMessage(GetDlgItem(hDlg, g_Keysets[i].able_id), BM_SETCHECK, BST_CHECKED, NULL);
                EnableWindow(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), TRUE);
            }
            break;
        default:
            break;
        }
        // add VK_SPACE support for hotkey control
        if (HIWORD(wParam) == EN_CHANGE)
        {
            if (SendMessage((HWND)lParam, HKM_GETHOTKEY, 0, 0) == 0)
            {
                ctrl_id = LOWORD(wParam);
                for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
                {
                    if (g_Keysets[i].ctrl_id == ctrl_id && LOWORD(g_Keysets[i].key) != KT_HOTKEY)
                    {
                        SendMessage(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), HKM_SETHOTKEY, MAKEWORD(VK_SPACE, 0), 0);
                        break;
                    }
                }
            }
        }
        for (i=KI_HIDE; i<KI_MAXCOUNT;i++)
        {
            if (LOWORD(wParam) == g_Keysets[i].able_id)
            {
                res = SendMessage(GetDlgItem(hDlg, g_Keysets[i].able_id), BM_GETCHECK, 0, NULL);
                if (res == BST_CHECKED)
                {
                    EnableWindow(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), TRUE);
                }
                else
                {
                    EnableWindow(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), FALSE);
                }
                break;
            }
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

static UINT HotkeyToMod(UINT value)
{
    value &= ~HOTKEYF_EXT;

    if ( (value & HOTKEYF_SHIFT) && !(value & HOTKEYF_ALT) )
    {
        value &= ~HOTKEYF_SHIFT;
        value |= MOD_SHIFT;
    }
    else if ( !(value & HOTKEYF_SHIFT) && (value & HOTKEYF_ALT) )
    {
        value &= ~HOTKEYF_ALT;
        value |= MOD_ALT;
    }
    return value;
}

DWORD ToHotkey(WPARAM wParam)
{
    DWORD hw = 0,lw = 0;

    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
    {
        hw |= HOTKEYF_CONTROL;
    }
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    {
        hw |= HOTKEYF_SHIFT;
    }
    if (GetAsyncKeyState(18) & 0x8000)
    {
        hw |= HOTKEYF_ALT;
    }

    if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN
        || wParam == VK_PRIOR || wParam == VK_NEXT)
    {
        hw |= HOTKEYF_EXT;
    }
    lw = (DWORD)wParam;

    return MAKEWORD(lw, hw);
}

BOOL KS_KeyDownProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD val;
    int i;

    val = ToHotkey(wParam);
    for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
    {
        if (LOWORD(g_Keysets[i].key) == KT_SHORTCUTKEY && val == (*g_Keysets[i].pvalue))
        {
            if (*g_Keysets[i].is_disable)
                return TRUE;
            g_Keysets[i].proc(hWnd, message, wParam, lParam);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL KS_HotKeyProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i;

    for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
    {
        if (LOWORD(g_Keysets[i].key) == KT_HOTKEY && g_Keysets[i].key_id == wParam)
        {
            if (*g_Keysets[i].is_disable)
                return TRUE;
            g_Keysets[i].proc(hWnd, message, wParam, lParam);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL _register_hotkey(HWND hWnd, DWORD kid, DWORD val, const TCHAR *dest)
{
    UINT uVK;
    UINT uMod;

    uVK = LOBYTE(LOWORD(val));
    uMod = HIBYTE(LOWORD(val));
    uMod = HotkeyToMod(uMod);
    if (!RegisterHotKey(hWnd, kid, uMod, uVK))
    {
        MessageBoxFmt_(hWnd, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_REGISTER_FAIL, dest);
        return FALSE;
    }
    return TRUE;
}

static BOOL _unregister_hotkey(HWND hWnd, DWORD kid)
{
    return UnregisterHotKey(hWnd, kid);
}

BOOL KS_RegisterAllHotKey(HWND hWnd)
{
    int i;
    TCHAR desc[256] = { 0 };

    for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
    {
        if (LOWORD(g_Keysets[i].key) == KT_HOTKEY)
        {
            LoadString(hInst, g_Keysets[i].desc, desc, 256);
            if (!_register_hotkey(hWnd, g_Keysets[i].key_id, *(g_Keysets[i].pvalue), desc))
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

BOOL KS_UnRegisterAllHotKey(HWND hWnd)
{
    int i;

    for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
    {
        if (LOWORD(g_Keysets[i].key) == KT_HOTKEY)
        {
            _unregister_hotkey(hWnd, g_Keysets[i].key_id);
        }
    }
    return TRUE;
}

#if ENABLE_GLOBAL_KEY
BOOL IsCoveredByOtherWindow(HWND hWnd)
{
    RECT rcTarget;
    RECT rcWnd;
    BOOL isChild;
    HWND hCurWnd;
    GetWindowRect(hWnd, &rcTarget);

    isChild = (WS_CHILD == (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD));

    if (GetDesktopWindow() == hWnd)
        hWnd = GetWindow(GetTopWindow(hWnd), GW_HWNDLAST);
    do
    {
        hCurWnd = hWnd;
        while (NULL != (hWnd = GetNextWindow(hWnd, GW_HWNDPREV)))
        {
            if (IsWindowVisible(hWnd))
            {
                GetWindowRect(hWnd, &rcWnd);
                if (!((rcWnd.right < rcTarget.left) || (rcWnd.left > rcTarget.right) ||
                    (rcWnd.bottom < rcTarget.top) || (rcWnd.top > rcTarget.bottom)))
                {
                    return TRUE;
                }
            }
        }
        if (isChild)
        {
            hWnd = GetParent(hCurWnd);
            isChild = hWnd ? (WS_CHILD == (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)) : FALSE;
        }
        else
        {
            break;
        }
    } while (TRUE);

    return FALSE;
}
#endif
