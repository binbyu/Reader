#include "Editctrl.h"
#include "Keyset.h"
#include "resource.h"
#include "Book.h"

static BOOL g_IsEditMode = FALSE;
static HWND g_hEditCtrl = NULL;
static WNDPROC g_defproc = NULL;
static HFONT g_hFont = NULL;
static TCHAR *g_text = NULL;

extern keydata_t g_Keysets[KI_MAXCOUNT];
extern Book *_Book;
extern HWND _hWnd;
extern window_info_t _WndInfo;
extern BOOL GetClientRectExceptStatusBar(HWND hWnd, RECT* rc);
extern DWORD ToHotkey(WPARAM wParam);
extern int MessageBox_(HWND, UINT, UINT, UINT);
extern LRESULT OnHideBorder(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK EditWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void EnableMenu(HMENU hMenu, BOOL enable);
static void EnableAllMenu(HWND hWnd, BOOL enable);

void EC_EnterEditMode(HINSTANCE hInst, HWND hWnd, LOGFONT *font, TCHAR *text, BOOL readonly)
{
    RECT rc;
    size_t len;

    if (!g_hEditCtrl)
    {
        g_hEditCtrl = CreateWindow(WC_EDIT, _T("Edit Ctrl"),
            /*WS_VISIBLE |*/ WS_CHILD /*| WS_BORDER*/ | ES_LEFT | ES_MULTILINE/* | ES_READONLY*/ | ES_AUTOVSCROLL,
            0, 0, 200, 300, hWnd, NULL, hInst, NULL);

        g_defproc = (WNDPROC)SetWindowLongPtr(g_hEditCtrl, GWLP_WNDPROC, (LONG_PTR)EditWndProc);
    }

    EnableAllMenu(hWnd, FALSE);

    GetClientRectExceptStatusBar(hWnd, &rc);

    if (g_hFont)
    {
        DeleteObject(g_hFont);
    }
    g_hFont = CreateFontIndirect(font);
    SetWindowPos(g_hEditCtrl, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOREDRAW);
    SendMessage(g_hEditCtrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hEditCtrl, WM_SETTEXT, 0, (LPARAM)text);
    SendMessage(g_hEditCtrl, EM_SETRECT, 0, (LPARAM)&rc);
    SendMessage(g_hEditCtrl, EM_SETREADONLY, (WPARAM)readonly, NULL);

    SetFocus(g_hEditCtrl);
    ShowWindow(g_hEditCtrl, SW_SHOW);

    // backup source text
    len = _tcslen(text);
    g_text = (TCHAR *)malloc(sizeof(TCHAR) * (len+1));
    memcpy(g_text, text, sizeof(TCHAR) * (len+1));

    g_IsEditMode = TRUE;
}

void EC_LeaveEditMode(void)
{
    int len;
    TCHAR *buffer;

    if (g_IsEditMode)
    {
        // save text
        len = (int)SendMessage(g_hEditCtrl, WM_GETTEXTLENGTH, 0, NULL);
        if (len >= 0)
        {
            buffer = (TCHAR*)malloc(sizeof(TCHAR)*(len+1));
            if (buffer)
            {
                buffer[len] = 0;
                if (len > 0)
                {
                    GetWindowText(g_hEditCtrl, buffer, len+1);
                }

                // is changed?
                if (0 != _tcscmp(buffer, g_text))
                {
                    if (IDYES == MessageBox_(g_hEditCtrl, IDS_SAVE_TEXT_TIPS, IDS_SAVE_FILE, MB_YESNO|MB_ICONWARNING))
                    {
                        if (!_Book->SetCurPageText(GetParent(g_hEditCtrl), buffer))
                        {
                            MessageBox_(g_hEditCtrl, IDS_SAVE_FAIL, IDS_SAVE_FILE, MB_OK|MB_ICONERROR);
                        }
                    }
                }

                free(buffer);
            }
        }

        ShowWindow(g_hEditCtrl, SW_HIDE);
        SetFocus(GetParent(g_hEditCtrl));
        DeleteObject(g_hFont);
        g_hFont = NULL;
        g_IsEditMode = FALSE;
        EnableAllMenu(GetParent(g_hEditCtrl), TRUE);
        if (g_text)
        {
            free(g_text);
            g_text = NULL;
        }

        if (_WndInfo.status_bak == ds_borderless)
        {
            OnHideBorder(_hWnd, 0, 0, 0);
        }
    }
}

BOOL EC_IsEditMode(void)
{
    return g_IsEditMode;
}

static LRESULT CALLBACK EditWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
        {
            DWORD val;

            val = ToHotkey(wParam);
            if (val == *(g_Keysets[KI_EDIT].pvalue))
            {
                EC_LeaveEditMode();
            }

            // add ctrl + s
            if ('S' == wParam)
            {
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
                {
                    EC_LeaveEditMode();
                }
            }
            // ctrl + a, select all
            else if ('A' == wParam)
            {
                SendMessage(hWnd, EM_SETSEL, 0, -1);
            }
        }
        break;
    case WM_MOUSEWHEEL:
    {
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
        {
            SendMessage(hWnd, EM_LINESCROLL, 0, -1);
        }
        else
        {
            SendMessage(hWnd, EM_LINESCROLL, 0, 1);
        }
    }
    break;
    }

    return CallWindowProc(g_defproc, hWnd, message, wParam, lParam);
}

void EnableMenu(HMENU hMenu, BOOL enable)
{
    HMENU hSubMenu;
    int count;
    int i;
    int menuid;

    if (!hMenu)
        return;

    count = GetMenuItemCount(hMenu);
    for (i=0; i<count; i++)
    {
        hSubMenu = GetSubMenu(hMenu, i);
        if (hSubMenu)
            EnableMenu(hSubMenu, enable);
        menuid = GetMenuItemID(hMenu, i);
        if (-1 != menuid)
            EnableMenuItem(hMenu, menuid, enable?MF_ENABLED:MF_DISABLED);
    }
}

void EnableAllMenu(HWND hWnd, BOOL enable)
{
    HMENU hMenu;

    hMenu = GetMenu(hWnd);
    EnableMenu(hMenu, enable);
    DrawMenuBar(hWnd);
}
