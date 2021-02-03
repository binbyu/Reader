#include "StdAfx.h"
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

static BOOL _register_hotkey(HWND hWnd, DWORD kid, DWORD val, const TCHAR *dest);
static BOOL _unregister_hotkey(HWND hWnd, DWORD kid);

keydata_t g_Keysets[KI_MAXCOUNT] = 
{
    { MAKELONG(KT_HOTKEY, KI_HIDE),             ID_HOTKEY_SHOW_HIDE_WINDOW, 0, MAKEWORD('H', HOTKEYF_ALT),                      IDC_HK_HIDE,        OnHideWin,     IDS_HIDE_SHOW_WINDOW },
    { MAKELONG(KT_SHORTCUTKEY, KI_BORDER),      0,                          0, MAKEWORD(VK_F12, 0),                             IDC_HK_BORDER,      OnHideBorder,  IDS_HIDE_SHOW_BORDER },
    { MAKELONG(KT_SHORTCUTKEY, KI_FULLSCREEN),  0,                          0, MAKEWORD(VK_F11, 0),                             IDC_HK_FULLSCREEN,  OnFullScreen,  IDS_FULLSRCEEN },
    { MAKELONG(KT_SHORTCUTKEY, KI_TOP),         0,                          0, MAKEWORD('T', HOTKEYF_CONTROL),                  IDC_HK_TOP,         OnTopmost,     IDS_TOPMOST },
    { MAKELONG(KT_SHORTCUTKEY, KI_OPEN),        0,                          0, MAKEWORD('O', HOTKEYF_CONTROL),                  IDC_HK_OPEN,        OnOpenFile,    IDS_OPEN_FILE },
    { MAKELONG(KT_SHORTCUTKEY, KI_ADDMARK),     0,                          0, MAKEWORD('M', HOTKEYF_CONTROL),                  IDC_HK_ADDMARK,     OnAddMark,     IDS_ADD_BOOKMARK },
    { MAKELONG(KT_SHORTCUTKEY, KI_AUTOPAGE),    0,                          0, MAKEWORD(VK_SPACE, 0),                           IDC_HK_AUTOPAGE,    OnAutoPage,    IDS_AUTO_PAGE },
    { MAKELONG(KT_SHORTCUTKEY, KI_SEARCH),      0,                          0, MAKEWORD('F', HOTKEYF_CONTROL),                  IDC_HK_SEARCH,      OnSearch,      IDS_TEXT_SEARCH },
    { MAKELONG(KT_SHORTCUTKEY, KI_JUMP),        0,                          0, MAKEWORD('G', HOTKEYF_CONTROL),                  IDC_HK_JUMP,        OnJump,        IDS_PROGRESS_JUMP },
    { MAKELONG(KT_SHORTCUTKEY, KI_EDIT),        0,                          0, MAKEWORD('E', HOTKEYF_CONTROL),                  IDC_HK_EDIT,        OnEditMode,    IDS_EDIT_MODE },
    { MAKELONG(KT_SHORTCUTKEY, KI_PAGEUP),      0,                          0, MAKEWORD(VK_LEFT, HOTKEYF_EXT),                  IDC_HK_PAGEUP,      OnPageUp,      IDS_PAGE_UP },
    { MAKELONG(KT_SHORTCUTKEY, KI_PAGEDOWN),    0,                          0, MAKEWORD(VK_RIGHT, HOTKEYF_EXT),                 IDC_HK_PAGEDOWN,    OnPageDown,    IDS_PAGE_DOWN },
    { MAKELONG(KT_SHORTCUTKEY, KI_LINEUP),      0,                          0, MAKEWORD(VK_UP, HOTKEYF_EXT),                    IDC_HK_LINEUP,      OnLineUp,      IDS_LINE_UP },
    { MAKELONG(KT_SHORTCUTKEY, KI_LINEDOWN),    0,                          0, MAKEWORD(VK_DOWN, HOTKEYF_EXT),                  IDC_HK_LINEDOWN,    OnLineDown,    IDS_LINE_DOWN },
    { MAKELONG(KT_SHORTCUTKEY, KI_CHAPTERUP),   0,                          0, MAKEWORD(VK_LEFT, HOTKEYF_EXT|HOTKEYF_CONTROL),  IDC_HK_CHAPTERUP,   OnChapterUp,   IDS_CHAPTER_UP },
    { MAKELONG(KT_SHORTCUTKEY, KI_CHAPTERDOWN), 0,                          0, MAKEWORD(VK_RIGHT, HOTKEYF_EXT|HOTKEYF_CONTROL), IDC_HK_CHAPTERDOWN, OnChapterDown, IDS_CHAPTER_DOWN },
    { MAKELONG(KT_SHORTCUTKEY, KI_FONTZOOMIN),  0,                          0, MAKEWORD(VK_OEM_PLUS, HOTKEYF_CONTROL),          IDC_HK_FONTZOOMIN,  OnFontZoomIn,  IDS_FONT_ZOOMIN },
    { MAKELONG(KT_SHORTCUTKEY, KI_FONTZOOMOUT), 0,                          0, MAKEWORD(VK_OEM_MINUS, HOTKEYF_CONTROL),         IDC_HK_FONTZOOMOUT, OnFontZoomOut, IDS_FONT_ZOOMOUT }
};


void KS_Init(HWND hWnd, void *keybuff)
{
    int i;
    DWORD *buff;

    buff = (DWORD *)keybuff;
    if (!buff)
        return;

    for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
    {
        g_Keysets[i].pvalue = &(buff[i]);
    }

    // register hot key
    KS_RegisterAllHotKey(hWnd);
}

void KS_UpdateBuffAddr(void *keybuff)
{
    int i;
    DWORD *buff;

    buff = (DWORD *)keybuff;
    if (!buff)
        return;

    for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
    {
        g_Keysets[i].pvalue = &(buff[i]);
    }
}

void KS_OpenDlg(HINSTANCE hInst, HWND hWnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_KEYSET), hWnd, KS_DlgProc);
}

INT_PTR CALLBACK KS_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i,j;
    DWORD tempkeys[KI_MAXCOUNT] = {0};
    TCHAR msg[256] = {0};
    int ctrl_id = 0;
    TCHAR desc[256] = { 0 };

    switch (message)
    {
    case WM_INITDIALOG:
        for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
        {
            SendMessage(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), HKM_SETHOTKEY, *(g_Keysets[i].pvalue), 0);
            if (LOWORD(g_Keysets[i].key) == KT_SHORTCUTKEY)
            {
                SendMessage(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), HKM_SETRULES, HKCOMB_A, 0);
            }
        }
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
            {
                tempkeys[i] = SendMessage(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), HKM_GETHOTKEY, 0, 0);
            }
            // check duplicate
            for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
            {
                for (j=i+1; j<KI_MAXCOUNT; j++)
                {
                    if (tempkeys[i] == 0)
                    {
                        MessageBoxFmt_(hDlg, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_KEYVALUE_EMPTY, g_Keysets[i].desc);
                        return (INT_PTR)FALSE;
                    }
                    if (tempkeys[i] == tempkeys[j])
                    {
                        MessageBoxFmt_(hDlg, IDS_ERROR, MB_ICONERROR | MB_OK, IDS_KEYVALUE_DUPLICATE, g_Keysets[i].desc);
                        return (INT_PTR)FALSE;
                    }
                }
            }
            // check hotkey
            for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
            {
                if (LOWORD(g_Keysets[i].key) == KT_HOTKEY)
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
            }
            // set value
            for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
            {
                *(g_Keysets[i].pvalue) = tempkeys[i];
            }
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
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

void KS_GetDefaultKeyBuff(void *keybuff)
{
    int i;
    DWORD *buff;

    buff = (DWORD *)keybuff;
    if (!buff)
        return;

    for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
    {
        buff[i] = g_Keysets[i].defval;
    }
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
    lw = wParam;

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
    TCHAR msg[256] = {0};

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

