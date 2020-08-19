#include "StdAfx.h"
#include "Editctrl.h"
#include "Keyset.h"
#include "resource.h"

static BOOL g_IsEditMode = FALSE;
static HWND g_hEditCtrl = NULL;
static WNDPROC g_defproc = NULL;
static HFONT g_hFont = NULL;

extern keydata_t g_Keysets[KI_MAXCOUNT];
extern BOOL GetClientRectExceptStatusBar(HWND hWnd, RECT* rc);
extern DWORD ToHotkey(WPARAM wParam);
static LRESULT CALLBACK EditWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void EnableMenu(HMENU hMenu, BOOL enable);
static void EnableAllMenu(HWND hWnd, BOOL enable);

void EC_EnterEditMode(HINSTANCE hInst, HWND hWnd, LOGFONT *font, TCHAR *text)
{
    RECT rc;

    if (!g_hEditCtrl)
    {
        g_hEditCtrl = CreateWindow(WC_EDIT, _T("Edit Ctrl"),
            /*WS_VISIBLE |*/ WS_CHILD /*| WS_BORDER*/ | ES_LEFT | ES_MULTILINE/* | ES_READONLY*/,
            0, 0, 200, 300, hWnd, NULL, hInst, NULL);

        g_defproc = (WNDPROC)SetWindowLongPtr(g_hEditCtrl, GWL_WNDPROC, (LONG_PTR)EditWndProc);
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

    SetFocus(g_hEditCtrl);
    ShowWindow(g_hEditCtrl, SW_SHOW);
    g_IsEditMode = TRUE;
}

void EC_LeaveEditMode(void)
{
    if (g_IsEditMode)
    {
        ShowWindow(g_hEditCtrl, SW_HIDE);
        SetFocus(GetParent(g_hEditCtrl));
        DeleteObject(g_hFont);
        g_hFont = NULL;
        g_IsEditMode = FALSE;
        EnableAllMenu(GetParent(g_hEditCtrl), TRUE);
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
    HMENU hSubMenu;
    int pos;

    hMenu = GetMenu(hWnd);
    EnableMenu(hMenu, enable);
    DrawMenuBar(hWnd);
}
