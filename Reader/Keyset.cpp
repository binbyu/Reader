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


static BOOL _register_hotkey(HWND hWnd, DWORD kid, DWORD val, const TCHAR *dest);
static BOOL _unregister_hotkey(HWND hWnd, DWORD kid);

keydata_t g_Keysets[KI_MAXCOUNT] = 
{
    { MAKELONG(KT_HOTKEY, KI_HIDE),             ID_HOTKEY_SHOW_HIDE_WINDOW, 0, MAKEWORD('H', HOTKEYF_ALT),                      IDC_HK_HIDE,        OnHideWin,     _T("隐藏/显示窗口") },
    { MAKELONG(KT_SHORTCUTKEY, KI_BORDER),      0,                          0, MAKEWORD(VK_F12, 0),                             IDC_HK_BORDER,      OnHideBorder,  _T("隐藏/显示边框") },
    { MAKELONG(KT_SHORTCUTKEY, KI_FULLSCREEN),  0,                          0, MAKEWORD(VK_F11, 0),                             IDC_HK_FULLSCREEN,  OnFullScreen,  _T("全屏/退出全屏") },
    { MAKELONG(KT_SHORTCUTKEY, KI_TOP),         0,                          0, MAKEWORD('T', HOTKEYF_CONTROL),                  IDC_HK_TOP,         OnTopmost,     _T("置顶/取消全屏") },
    { MAKELONG(KT_SHORTCUTKEY, KI_OPEN),        0,                          0, MAKEWORD('O', HOTKEYF_CONTROL),                  IDC_HK_OPEN,        OnOpenFile,    _T("打开文件") },
    { MAKELONG(KT_SHORTCUTKEY, KI_ADDMARK),     0,                          0, MAKEWORD('M', HOTKEYF_CONTROL),                  IDC_HK_ADDMARK,     OnAddMark,     _T("添加书签") },
    { MAKELONG(KT_SHORTCUTKEY, KI_AUTOPAGE),    0,                          0, MAKEWORD(VK_SPACE, 0),                           IDC_HK_AUTOPAGE,    OnAutoPage,    _T("自动翻页") },
    { MAKELONG(KT_SHORTCUTKEY, KI_SEARCH),      0,                          0, MAKEWORD('F', HOTKEYF_CONTROL),                  IDC_HK_SEARCH,      OnSearch,      _T("全文查找") },
    { MAKELONG(KT_SHORTCUTKEY, KI_JUMP),        0,                          0, MAKEWORD('G', HOTKEYF_CONTROL),                  IDC_HK_JUMP,        OnJump,        _T("进度跳转") },
    { MAKELONG(KT_SHORTCUTKEY, KI_EDIT),        0,                          0, MAKEWORD('E', HOTKEYF_CONTROL),                  IDC_HK_EDIT,        OnEditMode,    _T("编辑模式") },
    { MAKELONG(KT_SHORTCUTKEY, KI_PAGEUP),      0,                          0, MAKEWORD(VK_LEFT, HOTKEYF_EXT),                  IDC_HK_PAGEUP,      OnPageUp,      _T("上一页") },
    { MAKELONG(KT_SHORTCUTKEY, KI_PAGEDOWN),    0,                          0, MAKEWORD(VK_RIGHT, HOTKEYF_EXT),                 IDC_HK_PAGEDOWN,    OnPageDown,    _T("下一页") },
    { MAKELONG(KT_SHORTCUTKEY, KI_LINEUP),      0,                          0, MAKEWORD(VK_UP, HOTKEYF_EXT),                    IDC_HK_LINEUP,      OnLineUp,      _T("上N行") },
    { MAKELONG(KT_SHORTCUTKEY, KI_LINEDOWN),    0,                          0, MAKEWORD(VK_DOWN, HOTKEYF_EXT),                  IDC_HK_LINEDOWN,    OnLineDown,    _T("下N行") },
    { MAKELONG(KT_SHORTCUTKEY, KI_CHAPTERUP),   0,                          0, MAKEWORD(VK_LEFT, HOTKEYF_EXT|HOTKEYF_CONTROL),  IDC_HK_CHAPTERUP,   OnChapterUp,   _T("上一章") },
    { MAKELONG(KT_SHORTCUTKEY, KI_CHAPTERDOWN), 0,                          0, MAKEWORD(VK_RIGHT, HOTKEYF_EXT|HOTKEYF_CONTROL), IDC_HK_CHAPTERDOWN, OnChapterDown, _T("下一章") }
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

    switch (message)
    {
    case WM_INITDIALOG:
        for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
        {
            SendMessage(GetDlgItem(hDlg, g_Keysets[i].ctrl_id), HKM_SETHOTKEY, *(g_Keysets[i].pvalue), 0);
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
                        _sntprintf(msg, 256, _T("【%s】 \r\n\r\n键值不能为空，请重新设置"), g_Keysets[i].desc);
                        MessageBox(hDlg, msg, _T("错误"), MB_ICONERROR|MB_OK);
                        return (INT_PTR)FALSE;
                    }
                    if (tempkeys[i] == tempkeys[j])
                    {
                        _sntprintf(msg, 256, _T("【%s】 \r\n\r\n键值重复，请重新设置"), g_Keysets[j].desc);
                        MessageBox(hDlg, msg, _T("错误"), MB_ICONERROR|MB_OK);
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
                        _unregister_hotkey(GetParent(hDlg), g_Keysets[i].key_id);
                        if (!_register_hotkey(GetParent(hDlg), g_Keysets[i].key_id, tempkeys[i], g_Keysets[i].desc))
                        {
                            _register_hotkey(GetParent(hDlg), g_Keysets[i].key_id, *(g_Keysets[i].pvalue), g_Keysets[i].desc);
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

    if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN)
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
        _sntprintf(msg, 256, _T("注册热键【%s】失败，可能发生冲突\r\n请至【设置】>【其他设置】内重新设置按键！"), dest);
        MessageBox(hWnd, msg, _T("错误"), MB_ICONERROR|MB_OK);
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

    for (i=KI_HIDE; i<KI_MAXCOUNT; i++)
    {
        if (LOWORD(g_Keysets[i].key) == KT_HOTKEY)
        {
            if (!_register_hotkey(hWnd, g_Keysets[i].key_id, *(g_Keysets[i].pvalue), g_Keysets[i].desc))
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

