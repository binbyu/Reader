#include "StdAfx.h"
#include "Editctrl.h"
#include "Keyset.h"

static BOOL g_IsEditMode = FALSE;
static HWND g_hEditCtrl = NULL;
static WNDPROC g_defproc = NULL;
static HFONT g_hFont = NULL;

extern keydata_t g_Keysets[KI_MAXCOUNT];
extern BOOL GetClientRectExceptStatusBar(HWND hWnd, RECT* rc);
extern DWORD ToHotkey(WPARAM wParam);
static LRESULT CALLBACK EditWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void EC_EnterEditMode(HINSTANCE hInst, HWND hWnd, LOGFONT *font, TCHAR *text)
{
    RECT rc;

    if (!g_hEditCtrl)
    {
        g_hEditCtrl = CreateWindow(WC_EDIT, _T("Edit Ctrl"),
            /*WS_VISIBLE |*/ WS_CHILD /*| WS_BORDER*/ | ES_LEFT | ES_MULTILINE | ES_READONLY,
            0, 0, 200, 300, hWnd, NULL, hInst, NULL);

        g_defproc = (WNDPROC)SetWindowLongPtr(g_hEditCtrl, GWL_WNDPROC, (LONG_PTR)EditWndProc);
    }

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
    //DeleteObject(hFont);
    g_IsEditMode = TRUE;
}

void EC_UpdateEditMode(HWND hWnd, TCHAR *text)
{
    RECT rc;

    if (g_hEditCtrl)
    {
        GetClientRectExceptStatusBar(hWnd, &rc);
        SetWindowPos(g_hEditCtrl, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOREDRAW);
        SendMessage(g_hEditCtrl, WM_SETTEXT, 0, (LPARAM)text);
    }
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