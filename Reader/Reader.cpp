// Reader.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Reader.h"
#include "TextBook.h"
#include "EpubBook.h"
#include "OnlineBook.h"
#include "Keyset.h"
#include "Editctrl.h"
#include "Advset.h"
#include "DPIAwareness.h"
#include "barcode.h"
#include "OnlineDlg.h"
#if ENABLE_TAG
#include "tagset.h"
#endif
#include <stdio.h>
#include <shlwapi.h>
#include <CommDlg.h>
#include <commctrl.h>
#include <vector>
#ifdef _DEBUG
#include "dump.h"
#endif

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "shell32.lib")

#define MIN_ALPHA_VALUE             0x64

#define MAX_LOADSTRING              256

// Global Variables:
HINSTANCE hInst;                                // current instance
TCHAR szTitle[MAX_LOADSTRING];                    // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
TCHAR szStatusClass[MAX_LOADSTRING];            // the status window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Setting(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Proxy(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    BgImage(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    JumpProgress(HWND, UINT, WPARAM, LPARAM);
#if ENABLE_NETWORK
INT_PTR CALLBACK    UpgradeProc(HWND, UINT, WPARAM, LPARAM);
#endif



int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

     // TODO: Place code here.
#ifdef _DEBUG
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
#endif
    MSG msg;
    HACCEL hAccelTable;
    HANDLE hMutex;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_READER, szWindowClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_STATUSBAR, szStatusClass, MAX_LOADSTRING);

    // check if application is already running
    hMutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, szTitle);
    if (hMutex == NULL && ERROR_FILE_NOT_FOUND == GetLastError())
    {
        hMutex = CreateMutex(NULL, TRUE, szTitle);
    }
    else
    {
        // There's another instance running.
        static int flag = 0;
        EnumWindows(EnumWindowsProc, (LPARAM)&flag);
        return 0;
    }

    // init com
    CoInitialize(NULL);

    // init gdiplus
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // load cache file
    if (!Init())
    {
        return FALSE;
    }

    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_READER));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!_hFindDlg || !IsDialogMessage(_hFindDlg, &msg))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    Exit();

    // uninit gdiplus
    GdiplusShutdown(gdiplusToken);

    // uninit com
    CoUninitialize();

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style            = CS_HREDRAW | CS_VREDRAW /*| CS_DBLCLKS*/;
    wcex.lpfnWndProc      = WndProc;
    wcex.cbClsExtra       = 0;
    wcex.cbWndExtra       = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BOOK));
    wcex.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = CreateSolidBrush(_header->bg_color);//(HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName     = MAKEINTRESOURCE(IDC_READER);
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm          = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_BOOK));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   // prepare for XP style controls
   InitCommonControls();

   // update for dpi
   UpdateLayoutForDpi(&_header->rect);
   UpdateFontForDpi(&_header->font);
#if ENABLE_TAG
   for (int i = 0; i < MAX_TAG_COUNT; i++)
   {
       UpdateFontForDpi(&_header->tags[i].font);
   }
#endif

   // fixed rect exception bug.
   if (ENABLE_TAG)
   {
       int sw,sh;
       sw = GetCxScreenForDpi();
       sh = GetCyScreenForDpi();
       if (_header->rect.left < 0 && _header->rect.right - _header->rect.left <= sw)
       {
           _header->rect.right -= _header->rect.left;
           _header->rect.left = 0;
       }
       if (_header->rect.right > sw && _header->rect.right - _header->rect.left <= sw)
       {
           _header->rect.left -= _header->rect.right - sw;
           _header->rect.right = sw;
       }
       if (_header->rect.top < 0 && _header->rect.bottom - _header->rect.top <= sh)
       {
           _header->rect.bottom -= _header->rect.top;
           _header->rect.top = 0;
       }
       if (_header->rect.bottom > sh && _header->rect.bottom - _header->rect.top <= sh)
       {
           _header->rect.top -= _header->rect.bottom - sh;
           _header->rect.bottom = sh;
       }
       if (_header->rect.left < 0 || _header->rect.top < 0
           || _header->rect.right > sw || _header->rect.bottom > sh)
       {
           // default rect
           _header->rect.left = (GetCxScreenForDpi() - DEFAULT_APP_WIDTH) / 2;
           _header->rect.top = (GetCyScreenForDpi() - DEFAULT_APP_HEIGHT) / 2;
           _header->rect.right = _header->rect.left + DEFAULT_APP_WIDTH;
           _header->rect.bottom = _header->rect.top + DEFAULT_APP_HEIGHT;
           UpdateLayoutForDpi(&_header->rect);
       }
   }
   // -- fixed end
   
   hWnd = CreateWindowEx(WS_EX_ACCEPTFILES | WS_EX_LAYERED, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      _header->rect.left, _header->rect.top, 
      _header->rect.right - _header->rect.left,
      _header->rect.bottom - _header->rect.top,
      NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
   _hWnd = hWnd;
   SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);

   ShowSysTray(hWnd, _header->show_systray);
   ShowInTaskbar(hWnd, !_header->hide_taskbar);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    LRESULT hit;
    POINT pt;
    RECT rc;
    static RECT s_rect;

    if (message == _uFindReplaceMsg)
    {
        OnFindText(hWnd, message, wParam, lParam);
        return 0;
    }
    if (WM_TASKBAR_CREATED == message)
    {
        ShowSysTray(hWnd, TRUE);
    }
    switch (message)
    {
    case WM_COMMAND:
        PauseAutoPage(hWnd);
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        if (wmId >= IDM_OPEN_BEGIN && wmId <= IDM_OPEN_END)
        {
            int item_id = wmId - IDM_OPEN_BEGIN;
            OnOpenItem(hWnd, item_id, FALSE);
            ResumeAutoPage(hWnd);
            break;
        }
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDM_OPEN:
            OnOpenFile(hWnd, message, wParam, lParam);
            break;
        case IDM_CLEAR:
            OnClearFileList(hWnd, message, wParam, lParam);
            break;
        case IDM_FONT:
            OnSetFont(hWnd, message, wParam, lParam);
            break;
        case IDM_COLOR:
            OnSetBkColor(hWnd, message, wParam, lParam);
            break;
        case IDM_IMAGE:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_BG_IMAGE), hWnd, BgImage);
            break;
        case IDM_CONFIG:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTING), hWnd, Setting);
            break;
        case IDM_KEYSET:
            KS_OpenDlg(hInst, hWnd);
            break;
        case IDM_ADVSET:
            ADV_OpenDlg(hInst, hWnd, &(_header->chapter_rule), _Book);
            break;
        case IDM_ONLINE:
            OpenOnlineDlg(hInst, hWnd);
            break;
#if ENABLE_TAG
        case IDM_TAGSET:
            TS_OpenDlg(hInst, hWnd, _header->tags);
            if (_Book && !_Book->IsLoading())
            {
                _Book->Reset(hWnd, TRUE);
            }
            break;
#endif
        case IDM_PROXY:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_PROXY), hWnd, Proxy);
            break;
        case IDM_DEFAULT:
            OnRestoreDefault(hWnd, message, wParam, lParam);
            break;
        case IDM_VIEW:
            if (IsWindowVisible(_hTreeView))
            {
                ShowWindow(_hTreeView, SW_HIDE);
            }
            else
            {
                if (_Book && !_Book->IsLoading())
                {
                    TVITEM tvi;
                    HTREEITEM hti;
                    int index = _Book->GetCurChapterIndex();
                    if (index == -1)
                    {
                        break;
                    }
                    else if (index == 0)
                    {
                        hti = TreeView_GetRoot(_hTreeView);
                        TreeView_EnsureVisible(_hTreeView, hti);
                        TreeView_SelectItem(_hTreeView, hti);
                    }
                    else
                    {
                        hti = TreeView_GetRoot(_hTreeView);
                        do 
                        {
                            tvi.mask       = TVIF_PARAM;
                            tvi.hItem      = hti;
                            TreeView_GetItem(_hTreeView, &tvi);
                            if (tvi.lParam == index)
                            {
                                TreeView_EnsureVisible(_hTreeView, hti);
                                TreeView_SelectItem(_hTreeView, hti);
                                break;
                            }
                            hti = TreeView_GetNextSibling(_hTreeView, hti);
                        } while (hti);
                    }

                    GetClientRectExceptStatusBar(hWnd, &rc);
                    SetWindowPos(_hTreeView, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
                    SetFocus(_hTreeView);
                }
            }
            break;
        case IDM_MARK:
            if (IsWindowVisible(_hTreeMark))
            {
                ShowWindow(_hTreeMark, SW_HIDE);
            }
            else
            {
                if (_Book && !_Book->IsLoading() && _item)
                {
                    if (_item->mark_size <= 0)
                        break;
                    GetClientRectExceptStatusBar(hWnd, &rc);
                    SetWindowPos(_hTreeMark, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
                    SetFocus(_hTreeMark);
                }
            }
            break;
        default:
            ResumeAutoPage(hWnd);
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        ResumeAutoPage(hWnd);
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case TVN_SELCHANGED:
            if (((LPNMHDR)lParam)->hwndFrom == _hTreeView)
            {
                if (IsWindowVisible(_hTreeView))
                {
                    HTREEITEM Selected;
                    TVITEM item;
                    Selected = TreeView_GetSelection(_hTreeView);
                    if (Selected)
                    {
                        item.mask       = TVIF_PARAM;
                        item.hItem      = Selected;
                        TreeView_GetItem(_hTreeView, &item);
                        if (_Book && !_Book->IsLoading())
                            _Book->JumpChapter(hWnd, item.lParam);
                        //ShowWindow(_hTreeView, SW_HIDE);
                        PostMessage(hWnd, WM_COMMAND, IDM_VIEW, NULL);
                    }
                }
            }
            break;
        case NM_RCLICK:
            if (((LPNMHDR)lParam)->hwndFrom == _hTreeMark)
            {
                HTREEITEM hItem = TreeView_GetNextItem(((LPNMHDR)lParam)->hwndFrom, 0, TVGN_DROPHILITE);
                if(hItem)
                    TreeView_SelectItem(((LPNMHDR)lParam)->hwndFrom, hItem);
            }
            break;
        case NM_DBLCLK:
            if (((LPNMHDR)lParam)->hwndFrom == _hTreeMark)
            {
                if (IsWindowVisible(_hTreeMark))
                {
                    HTREEITEM Selected;
                    TVITEM item;
                    Selected = TreeView_GetSelection(_hTreeMark);
                    if (Selected)
                    {
                        item.mask       = TVIF_PARAM;
                        item.hItem      = Selected;
                        TreeView_GetItem(_hTreeMark, &item);
                        if (_Book && !_Book->IsLoading() && _item)
                        {
                            _item->index = _item->mark[item.lParam];
                            _Book->Reset(hWnd);
                        }
                        //ShowWindow(_hTreeMark, SW_HIDE);
                        PostMessage(hWnd, WM_COMMAND, IDM_MARK, NULL);
                    }
                }
            }
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_CONTEXTMENU:
        if (wParam == (WPARAM)_hTreeMark)
        {
            POINT pt;
            HTREEITEM Selected;
            TVITEM item;
            pt.x = LOWORD(lParam); 
            pt.y = HIWORD(lParam);
            Selected = TreeView_GetSelection(_hTreeMark);
            if (Selected)
            {
                HMENU hMenu = CreatePopupMenu();
                if (hMenu)
                {
                    InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_MK_DEL, _T("Delete"));
                    SetForegroundWindow(hWnd);
                    int ret = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, _hTreeMark, NULL);
                    DestroyMenu(hMenu);
                    if (IDM_MK_DEL == ret)
                    {
                        item.mask = TVIF_PARAM;
                        item.hItem = Selected;
                        TreeView_GetItem(_hTreeMark, &item);

                        if (_Cache.del_mark(_item, item.lParam))
                            OnUpdateBookMark(hWnd);
                    }
                }
            }
        }
        break;
    case WM_CREATE:
        OnCreate(hWnd);
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
        {
            ResetLayerd(hWnd);
            OnDraw(hWnd);
        }
        else
        {
            ResetLayerd(hWnd);
            OnPaint(hWnd, hdc);
        }
        if (_NeedSave)
        {
            _NeedSave = FALSE;
            Save(hWnd);
        }
        EndPaint(hWnd, &ps);
        break;
    case WM_CLOSE:
        StopAutoPage(hWnd);
        if (_header->show_systray)
        {
            _tcscpy(_nid.szInfoTitle, _T("Reader"));
            _tcscpy(_nid.szInfo, _T("I'm here!"));
            _nid.uTimeout = 1000;
            Shell_NotifyIcon(NIM_MODIFY, &_nid);
            ShowHideWindow(hWnd);
        }
        else
        {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_DESTROY:
        ShowSysTray(hWnd, FALSE);
        StopAutoPage(hWnd);
        KS_UnRegisterAllHotKey(hWnd);
        if (_hMouseHook)
        {
            UnhookWindowsHookEx(_hMouseHook);
            _hMouseHook = NULL;
        }
        // save rect
        if (_WndInfo.bHideBorder && !_WndInfo.bFullScreen)
        {
            GetClientRectExceptStatusBar(hWnd, &rc);
            ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.left));
            ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.right));
            rc.left = rc.left - _WndInfo.hbRect.left;
            rc.right = rc.right + _WndInfo.hbRect.right;
            rc.top = rc.top - _WndInfo.hbRect.top;
            rc.bottom = rc.bottom + _WndInfo.hbRect.bottom;
            _header->rect.left = rc.left;
            _header->rect.top = rc.top;
            _header->rect.right = rc.right;
            _header->rect.bottom = rc.bottom;
        }
        RestoreRectForDpi(&_header->rect);
        RestoreFontForDpi(&_header->font);
#if ENABLE_TAG
        for (int i=0; i<MAX_TAG_COUNT; i++)
        {
            RestoreFontForDpi(&_header->tags[i].font);
        }
#endif
        // stop loading
        StopLoadingImage(hWnd);
        if (_loading)
        {
            if (_loading->hMemory)
                ::GlobalFree(_loading->hMemory);
            if (_loading->image)
                delete _loading->image;
            free(_loading->item);
            free(_loading);
            _loading = NULL;
        }
        // close book
        if (_Book)
        {
            delete _Book;
            _Book = NULL;
        }
        PostQuitMessage(0);
        break;
    case WM_NCHITTEST:
        hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit == HTCLIENT && _WndInfo.bHideBorder && !_WndInfo.bFullScreen)
        {
            if (_header->page_mode == 2)
            {
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                GetClientRectExceptStatusBar(hWnd, &rc);
                rc.left = rc.right/3;
                rc.right = rc.right/3 * 2;
                if (PtInRect(&rc, pt))
                    hit = HTCAPTION;
            }
            else
            {
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                GetClientRectExceptStatusBar(hWnd, &rc);
                rc.bottom = rc.bottom/2 > 80 ? 80 : rc.bottom/2;
                if (PtInRect(&rc, pt))
                    hit = HTCAPTION;
            }
        }
        return hit;
    case WM_SIZING:
        if (EC_IsEditMode())
        {
            RECT *lprc = NULL;
            lprc = (RECT *)lParam;
            lprc->left = s_rect.left;
            lprc->top = s_rect.top;
            lprc->right = s_rect.right;
            lprc->bottom = s_rect.bottom;
            return TRUE;
        }
        break;
    case WM_SIZE:
        OnSize(hWnd, message, wParam, lParam);
        if (!IsZoomed(hWnd) && !IsIconic(hWnd) && !_WndInfo.bHideBorder && !_WndInfo.bFullScreen)
            GetWindowRect(hWnd, &_header->rect);
        GetWindowRect(hWnd, &s_rect);
        ResetAutoPage(hWnd);
        break;
    case WM_MOVE:
        OnMove(hWnd);
        if (!IsZoomed(hWnd) && !IsIconic(hWnd) && !_WndInfo.bHideBorder && !_WndInfo.bFullScreen)
            GetWindowRect(hWnd, &_header->rect);
        break;
    case WM_WINDOWPOSCHANGED:
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_KEYDOWN:
        if (_Book && _Book->IsLoading())
            break;

        if (KS_KeyDownProc(hWnd, message, wParam, lParam))
            break;

        if (VK_ESCAPE == wParam)
        {
            if (_WndInfo.bFullScreen)
            {
                OnFullScreen(hWnd, message, wParam, lParam);
            }
        }
        else if ('P' == wParam)
        {
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000
                && GetAsyncKeyState(VK_SHIFT) & 0x8000
                && GetAsyncKeyState(18) & 0x8000) // alt
            {
                if (!_hMouseHook)
                    _hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hInst, NULL);
                else
                {
                    UnhookWindowsHookEx(_hMouseHook);
                    _hMouseHook = NULL;
                }
            }
        }
        break;
    case WM_HOTKEY:
        KS_HotKeyProc(hWnd, message, wParam, lParam);
        break;
    case WM_LBUTTONDOWN:
        if ((wParam & MK_LBUTTON) && (wParam & MK_RBUTTON) && _header->disable_lrhide == 0)
        {
            ShowHideWindow(hWnd);
            break;
        }
        if (_header->page_mode == 1)
        {
            OnPageDown(hWnd, message, wParam, lParam);
        }
        else if (_header->page_mode == 2)
        {
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            GetClientRectExceptStatusBar(hWnd, &rc);
            rc.left = rc.right/3 * 2;
            if (PtInRect(&rc, pt))
            {
                OnPageDown(hWnd, message, wParam, lParam);
            }
            else
            {
                rc.left = 0;
                rc.right = rc.right/3;
                if (PtInRect(&rc, pt))
                {
                    OnPageUp(hWnd, message, wParam, lParam);
                }
            }
        }
        break;
    case WM_RBUTTONDOWN:
        if ((wParam & MK_LBUTTON) && (wParam & MK_RBUTTON) && _header->disable_lrhide == 0)
        {
            ShowHideWindow(hWnd);
            break;
        }
        if (_header->page_mode == 1)
        {
            OnPageUp(hWnd, message, wParam, lParam);
        }
        break;
    case WM_NCLBUTTONDOWN:
        if (_hMouseHook)
        {
            switch (wParam)
            {
            case HTMINBUTTON:
                //PostMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                return FALSE;
            case HTMAXBUTTON:
                //PostMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
                return FALSE;
            case HTCLOSE:
                UnhookWindowsHookEx(_hMouseHook);
                _hMouseHook = NULL;
                break;
            default:
                break;
            }
        }
        if (IsWindowVisible(_hTreeView))
        {
            ShowWindow(_hTreeView, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 1, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        if (IsWindowVisible(_hTreeMark))
        {
            ShowWindow(_hTreeMark, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 2, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_NCRBUTTONDOWN:
        if (IsWindowVisible(_hTreeView))
        {
            ShowWindow(_hTreeView, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 1, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        if (IsWindowVisible(_hTreeMark))
        {
            ShowWindow(_hTreeMark, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 2, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_MOUSEWHEEL:
        {
            const BYTE MIN_ALPHA = 0x01;
            const BYTE MAX_ALPHA = 0xff;
            const BYTE UNIT_STEP = 0x05;
            if (IsWindowVisible(_hTreeView))
            {
                SetFocus(_hTreeView);
                break;
            }
            if (IsWindowVisible(_hTreeMark))
            {
                SetFocus(_hTreeMark);
                break;
            }
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
            {
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
                {
                    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                    {
                        _header->alpha = MAX_ALPHA;
                        if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                        {
                            ResetLayerd(hWnd);
                            OnDraw(hWnd);
                        }
                        else
                        {
                            ResetLayerd(hWnd);
                            SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                        }
                    }
                    else
                    {
                        if (_header->alpha < MAX_ALPHA - UNIT_STEP)
                            _header->alpha += UNIT_STEP;
                        else
                            _header->alpha = MAX_ALPHA;
                        if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                        {
                            ResetLayerd(hWnd);
                            OnDraw(hWnd);
                        }
                        else
                        {
                            ResetLayerd(hWnd);
                            SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                        }
                    }
                }
                else if (GetAsyncKeyState(18) & 0x8000) // alt
                {
                    if (_textAlpha < MAX_ALPHA - UNIT_STEP)
                        _textAlpha += UNIT_STEP;
                    else
                        _textAlpha = MAX_ALPHA;
                    if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                    {
                        ResetLayerd(hWnd);
                        OnDraw(hWnd);
                    }
                    else
                    {
                        ResetLayerd(hWnd);
                        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                    }
                }
                else
                {
                    OnLineUp(hWnd, message, wParam, lParam);
                }
            }
            else
            {
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
                {
                    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                    {
                        _header->alpha = MIN_ALPHA;
                        if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                        {
                            ResetLayerd(hWnd);
                            OnDraw(hWnd);
                        }
                        else
                        {
                            ResetLayerd(hWnd);
                            SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                        }
                    }
                    else
                    {
                        if (_header->alpha > MIN_ALPHA + UNIT_STEP)
                            _header->alpha -= UNIT_STEP;
                        else
                            _header->alpha = MIN_ALPHA;
                        if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                        {
                            ResetLayerd(hWnd);
                            OnDraw(hWnd);
                        }
                        else
                        {
                            ResetLayerd(hWnd);
                            SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                        }
                    }
                }
                else if (GetAsyncKeyState(18) & 0x8000) // alt
                {
                    if (_textAlpha > 0x0f + UNIT_STEP)
                        _textAlpha -= UNIT_STEP;
                    else
                        _textAlpha = 0x0f;
                    if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                    {
                        ResetLayerd(hWnd);
                        OnDraw(hWnd);
                    }
                    else
                    {
                        ResetLayerd(hWnd);
                        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                    }
                }
                else
                {
                    OnLineDown(hWnd, message, wParam, lParam);
                }
            }
        }
        break;
    case WM_ERASEBKGND:
        if (!_Book)
            DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_DROPFILES:
        OnDropFiles(hWnd, message, wParam, lParam);
        break;
    case WM_TIMER:
        switch (wParam)
        {
        case IDT_TIMER_PAGE:
            if ((_header->autopage_mode & 0x0f) == apm_page)
                OnPageDown(hWnd, message, wParam, lParam);
            else
                OnLineDown(hWnd, message, wParam, lParam);
            if (_Book && _Book->IsLastPage())
                StopAutoPage(hWnd);
            else
                ResetAutoPage(hWnd);
            break;
#if ENABLE_NETWORK
        case IDT_TIMER_UPGRADE:
            _Upgrade.Check(UpgradeCallback, hWnd);
            break;
        case IDT_TIMER_CHECKBOOK:
            OnCheckBookUpdate(hWnd);
            break;
#endif
        case IDT_TIMER_LOADING:
            {
                GUID Guid;
                KillTimer(hWnd, IDT_TIMER_LOADING);
                Guid = FrameDimensionTime;
                _loading->image->SelectActiveFrame(&Guid, _loading->frameIndex);
                SetTimer(hWnd, IDT_TIMER_LOADING, ((UINT*)_loading->item->value)[_loading->frameIndex] * 10, NULL);
                _loading->frameIndex = (++ _loading->frameIndex) % _loading->frameCount;
                GetClientRectExceptStatusBar(hWnd, &rc);
                InvalidateRect(hWnd, &rc, FALSE);
            }
            break;
        default:
            break;
        }
        break;
    case WM_UPDATE_CHAPTERS:
        OnUpdateChapters(hWnd);
        OnUpdateBookMark(hWnd);
        break;
#if ENABLE_NETWORK
    case WM_NEW_VERSION:
        DialogBox(hInst, MAKEINTRESOURCE(IDD_UPGRADE), hWnd, UpgradeProc);
        break;
#endif
    case WM_OPEN_BOOK:
        OnOpenBookResult(hWnd, wParam == 1);
        break;
    case WM_BOOK_EVENT:
        {
            book_event_data_t* be = (book_event_data_t*)lParam;
            if (_Book && (!be || _Book == be->_this))
                _Book->OnBookEvent(hWnd, message, wParam, lParam);
#if ENABLE_NETWORK
            else
            {
                chkbook_arg_t* arg = GetCheckBookArguments();
                book_event_data_t* be = (book_event_data_t*)lParam;
                if (arg && arg->book && be && arg->book == be->_this)
                    arg->book->OnBookEvent(hWnd, message, wParam, lParam);
            }
#endif
        }
        break;
    case WM_SAVE_CACHE:
        OnSave(hWnd);
        break;
    case WM_SYSTRAY:
        switch(lParam)
        {
        case WM_LBUTTONUP:
            if (!IsWindowVisible(hWnd))
                ShowWindow(hWnd, SW_SHOW);
            if (IsIconic(hWnd))
                ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            break;
        case WM_RBUTTONDOWN:
        case WM_CONTEXTMENU:
            {
                int ret;
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                if(hMenu)
                {
                    InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_ST_OPEN, _T("Open"));
                    InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_ST_EXIT, _T("Exit"));
                    SetForegroundWindow(hWnd);
                    ret = TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                    DestroyMenu(hMenu);
                    if (IDM_ST_OPEN == ret)
                    {
                        if (!IsWindowVisible(hWnd))
                            ShowWindow(hWnd, SW_SHOW);
                        if (IsIconic(hWnd))
                            ShowWindow(hWnd, SW_RESTORE);
                        SetForegroundWindow(hWnd);
                    }
                    else if (IDM_ST_EXIT == ret)
                    {
                        DestroyWindow(hWnd);
                    }
                }
            }
            break;
        }
        break;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        {
            hdc = (HDC)wParam;
            SetTextColor(hdc, _header->font_color);
            SetBkColor(hdc, _header->bg_color);
            SetDCBrushColor(hdc, _header->bg_color);
            return (LRESULT) GetStockObject(DC_BRUSH);
        }
        break;
    case WM_QUERYENDSESSION:
        //Exit(); // save data when poweroff ?
        return TRUE;
    case WM_ENDSESSION:
        {
            // save rect
            if (_WndInfo.bHideBorder && !_WndInfo.bFullScreen)
            {
                GetClientRectExceptStatusBar(hWnd, &rc);
                ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.left));
                ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.right));
                rc.left = rc.left - _WndInfo.hbRect.left;
                rc.right = rc.right + _WndInfo.hbRect.right;
                rc.top = rc.top - _WndInfo.hbRect.top;
                rc.bottom = rc.bottom + _WndInfo.hbRect.bottom;
                _header->rect.left = rc.left;
                _header->rect.top = rc.top;
                _header->rect.right = rc.right;
                _header->rect.bottom = rc.bottom;
            }
            RestoreRectForDpi(&_header->rect);
            RestoreFontForDpi(&_header->font);
#if ENABLE_TAG
            for (int i=0; i<MAX_TAG_COUNT; i++)
            {
                RestoreFontForDpi(&_header->tags[i].font);
            }
#endif
            Exit();
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case 0x02E0: //WM_DPICHANGED
        {            
            NONCLIENTMETRICS theMetrics;
            theMetrics.cbSize = sizeof(NONCLIENTMETRICS);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS), (PVOID) &theMetrics,0);
            HFONT hFont = CreateFontIndirect(&(theMetrics.lfMenuFont));
            SendMessage(_hTreeView, WM_SETFONT, (WPARAM)hFont, NULL);
            SendMessage(_hTreeView, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
            SendMessage(_hTreeMark, WM_SETFONT, (WPARAM)hFont, NULL);
            SendMessage(_hTreeMark, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
            DpiChanged(hWnd, &_header->font, &_header->rect, wParam, (RECT*)lParam);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

Gdiplus::Bitmap *LoadBarCode(void)
{
    static Bitmap *image = NULL;
    IStream* pStream = NULL;
    void *buffer = NULL;
    HGLOBAL hMemory = NULL;

    if (!image)
    {
        hMemory = ::GlobalAlloc(GMEM_MOVEABLE, BC_IMAGE_LENGHT);
        if (!hMemory)
        {
            return image;
        }
        buffer = ::GlobalLock(hMemory);
        if (!buffer)
        {
            ::GlobalFree(hMemory);
            return image;
        }
        CopyMemory(buffer, bc_image, BC_IMAGE_LENGHT);
        if (::CreateStreamOnHGlobal(hMemory, FALSE, &pStream) == S_OK)
        {
            image = Gdiplus::Bitmap::FromStream(pStream);
            pStream->Release();
        }
        ::GlobalUnlock(hMemory);
        //::GlobalFree(hMemory);
    }

    if (!image || Gdiplus::Ok != image->GetLastStatus())
    {
        delete image;
        image = NULL;
        ::GlobalFree(hMemory);
    }

    return image;
}

INT_PTR CALLBACK RewardProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    Gdiplus::Bitmap *image = NULL;
    RECT client;
    double dpiscaled;

    switch (message)
    {
    case WM_INITDIALOG:
        image = LoadBarCode();
        if (image)
        {
            dpiscaled = GetDpiScaled();
            if (dpiscaled <= 1.0f)
            {
                ::SetRect(&client, 0, 0, (int)(image->GetWidth()/2.0f), (int)(image->GetHeight()/2.0f));
            }
            else if (dpiscaled <= 1.25)
            {
                ::SetRect(&client, 0, 0, (int)(image->GetWidth()/3.0f*2.0f), (int)(image->GetHeight()/3.0f*2.0f));
            }
            else
            {
                ::SetRect(&client, 0, 0, image->GetWidth(), image->GetHeight());
            }
            AdjustWindowRect(&client, GetWindowLongPtr(hDlg, GWL_STYLE), FALSE);
            SetWindowPos(hDlg, NULL, client.left, client.top, client.right-client.left, client.bottom-client.top, SWP_NOMOVE);
        }
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            Gdiplus::Graphics *graphics = NULL;
            Rect rect;
            
            hdc = BeginPaint(hDlg, &ps);
            // TODO: Add any drawing code here...
            image = LoadBarCode();
            if (!image || Gdiplus::Ok != image->GetLastStatus())
                return (INT_PTR)FALSE;

            GetClientRect(hDlg, &client);
            
            // draw image
            graphics = new Graphics(hdc);
            if (graphics)
            {
                rect.X = 0;
                rect.Y = 0;
                rect.Width = client.right-client.left;
                rect.Height = client.bottom-client.top;
                graphics->SetInterpolationMode(InterpolationModeHighQualityBicubic);
                graphics->DrawImage(image, rect, 0, 0, image->GetWidth(), image->GetHeight(), UnitPixel);
                delete graphics;
            }

            EndPaint(hDlg, &ps);
            return (INT_PTR)TRUE;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    HWND hWnd;
    RECT rc;

    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDC_BUTTON_REWARD)
        {
            if (LoadBarCode())
            {
                DialogBox(hInst, MAKEINTRESOURCE(IDD_REWARD), hDlg, RewardProc);
            }
        }
        break;
    case WM_LBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        hWnd = GetDlgItem(hDlg, IDC_STATIC_LINK);
        ClientToScreen(hDlg, &pt);
        GetWindowRect(hWnd, &rc);
        if (PtInRect(&rc, pt))
        {
#if ENABLE_NETWORK
            ShellExecute(NULL, _T("open"), _T("https://github.com/binbyu/Reader"), NULL, NULL, SW_SHOWNORMAL);
#endif
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Setting(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = FALSE;
    BOOL bUpdated = FALSE;
    int value = 0;
    int cid;
    LRESULT res;
    BYTE temp;
    TCHAR buf[256] = { 0 };
    RECT rc = { 0 };
    switch (message)
    {
    case WM_INITDIALOG:
        _stprintf(buf, _T("%d,%d,%d,%d"), _header->internal_border.left, _header->internal_border.top, _header->internal_border.right, _header->internal_border.bottom);
        SetDlgItemInt(hDlg, IDC_EDIT_LINEGAP, _header->line_gap, FALSE);
        SetDlgItemText(hDlg, IDC_EDIT_BORDER, buf);
        SetDlgItemInt(hDlg, IDC_EDIT_ELAPSE, _header->uElapse, TRUE);
        if (_header->page_mode == 0)
            cid = IDC_RADIO_MODE1;
        else if (_header->page_mode == 1)
            cid = IDC_RADIO_MODE2;
        else
            cid = IDC_RADIO_MODE3;
        SendMessage(GetDlgItem(hDlg, cid), BM_SETCHECK, BST_CHECKED, NULL);
        WheelSpeedInit(hDlg);
        // init window style
        if (_header->show_systray)
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_SETCHECK, BST_CHECKED, NULL);
        else
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_SETCHECK, BST_UNCHECKED, NULL);
        if (_header->hide_taskbar)
        {
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_TASKBAR), BM_SETCHECK, BST_CHECKED, NULL);
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_SETCHECK, BST_CHECKED, NULL);
            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRAY), FALSE);
        }
        else
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_TASKBAR), BM_SETCHECK, BST_UNCHECKED, NULL);
        if (_header->disable_lrhide)
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_LRHIDE), BM_SETCHECK, BST_UNCHECKED, NULL);
        else
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_LRHIDE), BM_SETCHECK, BST_CHECKED, NULL);
        if ((_header->autopage_mode & 0x0f) == apm_page)
            SendMessage(GetDlgItem(hDlg, IDC_RADIO_ATPAGE), BM_SETCHECK, BST_CHECKED, NULL);
        else
            SendMessage(GetDlgItem(hDlg, IDC_RADIO_ATWHEEL), BM_SETCHECK, BST_CHECKED, NULL);
        LoadString(hInst, IDS_FIXED_TIME, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_APM), CB_ADDSTRING, 0, (LPARAM)buf);
        LoadString(hInst, IDS_DYNAMIC_CALC, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_APM), CB_ADDSTRING, 0, (LPARAM)buf);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_APM), CB_SETCURSEL, _header->autopage_mode >> 8, NULL);
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_MENU_FONT), BM_SETCHECK, _header->meun_font_follow ? BST_CHECKED : BST_UNCHECKED, NULL);
        if (_header->word_wrap)
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_WORD_WRAP), BM_SETCHECK, BST_CHECKED, NULL);
        else
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_WORD_WRAP), BM_SETCHECK, BST_UNCHECKED, NULL);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            GetDlgItemInt(hDlg, IDC_EDIT_LINEGAP, &bResult, FALSE);
            if (!bResult)
            {
                MessageBox_(hDlg, IDS_INVALID_LINEGAP, IDS_ERROR, MB_OK|MB_ICONERROR);
                return (INT_PTR)TRUE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BORDER, buf, 256);
            value = _stscanf(buf, _T("%d,%d,%d,%d"), &rc.left, &rc.top, &rc.right, &rc.bottom);
            if (value != 4)
            {
                MessageBox_(hDlg, IDS_INVALID_INBORDER, IDS_ERROR, MB_OK|MB_ICONERROR);
                return (INT_PTR)TRUE;
            }
            value = GetDlgItemInt(hDlg, IDC_EDIT_ELAPSE, &bResult, FALSE);
            if (!bResult || value == 0)
            {
                MessageBox_(hDlg, IDS_INVALID_AUTOPAGE, IDS_ERROR, MB_OK|MB_ICONERROR);
                return (INT_PTR)TRUE;
            }
            if (SendMessage(GetDlgItem(hDlg, IDC_COMBO_SPEED), CB_GETCURSEL, 0, NULL) != -1)
            {
                _header->wheel_speed = SendMessage(GetDlgItem(hDlg, IDC_COMBO_SPEED), CB_GETCURSEL, 0, NULL) + 1;
            }
            res = SendMessage(GetDlgItem(hDlg, IDC_RADIO_MODE1), BM_GETCHECK, 0, NULL);
            if (res == BST_CHECKED)
            {
                _header->page_mode = 0;
            }
            else
            {
                res = SendMessage(GetDlgItem(hDlg, IDC_RADIO_MODE2), BM_GETCHECK, 0, NULL);
                if (res == BST_CHECKED)
                {
                    _header->page_mode = 1;
                }
                else
                {
                    _header->page_mode = 2;
                }
            }
            value = GetDlgItemInt(hDlg, IDC_EDIT_LINEGAP, &bResult, FALSE);
            if (value != _header->line_gap)
            {
                _header->line_gap = value;
                bUpdated = TRUE;
            }
            GetDlgItemText(hDlg, IDC_EDIT_BORDER, buf, 256);
            _stscanf(buf, _T("%d,%d,%d,%d"), &rc.left, &rc.top, &rc.right, &rc.bottom);
            if (memcmp(&rc, &_header->internal_border, sizeof(RECT)))
            {
                memcpy(&_header->internal_border, &rc, sizeof(RECT));
                bUpdated = TRUE;
            }
            value = GetDlgItemInt(hDlg, IDC_EDIT_ELAPSE, &bResult, FALSE);
            if (value != _header->uElapse)
            {
                _header->uElapse = value;
            }
            // show taskbar/tray or not
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TASKBAR), BM_GETCHECK, 0, NULL);
            if ((res == BST_CHECKED && _header->hide_taskbar == 0)
                || (res == BST_UNCHECKED && _header->hide_taskbar == 1))
            {
                _header->hide_taskbar = !_header->hide_taskbar;
                ShowInTaskbar(GetParent(hDlg), !_header->hide_taskbar);
            }
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_GETCHECK, 0, NULL);
            if ((res == BST_CHECKED && _header->show_systray == 0)
                || (res == BST_UNCHECKED && _header->show_systray == 1))
            {
                _header->show_systray = !_header->show_systray;
                ShowSysTray(GetParent(hDlg), _header->show_systray);
            }
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_LRHIDE), BM_GETCHECK, 0, NULL);
            _header->disable_lrhide = res == BST_CHECKED ? 0 : 1;
            res = SendMessage(GetDlgItem(hDlg, IDC_RADIO_ATPAGE), BM_GETCHECK, 0, NULL);
            _header->autopage_mode = 0;
            _header->autopage_mode |= res == BST_CHECKED ? apm_page : apm_line;
            res = SendMessage(GetDlgItem(hDlg, IDC_COMBO_APM), CB_GETCURSEL, 0, NULL);
            _header->autopage_mode |= res == 1 ? apm_count : apm_fixed;
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_MENU_FONT), BM_GETCHECK, 0, NULL);
            temp = res == BST_CHECKED ? 1 : 0;
            if (temp != _header->meun_font_follow)
            {
                _header->meun_font_follow = temp;
                SetTreeviewFont();
            }
            _header->meun_font_follow = temp;
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_WORD_WRAP), BM_GETCHECK, 0, NULL);
            value = res == BST_CHECKED ? 1 : 0;
            if (value != _header->word_wrap)
            {
                _header->word_wrap = value;
                bUpdated = TRUE;
            }
            if (bUpdated)
            {
                if (_Book)
                    _Book->Reset(GetParent(hDlg));
            }
            Save(GetParent(hDlg));
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_CHECK_TASKBAR:
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TASKBAR), BM_GETCHECK, 0, NULL);
            if (res == BST_CHECKED)
            {
                SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_SETCHECK, BST_CHECKED, NULL);
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRAY), FALSE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRAY), TRUE);
            }
            break;
        case IDC_CHECK_TRAY:
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

INT_PTR CALLBACK Proxy(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT res;
    BOOL bResult = FALSE;
    int value = 0;
    TCHAR buf[256] = {0};
    switch (message)
    {
    case WM_INITDIALOG:
        LoadString(hInst, IDS_DISABLE_PROXY, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_PROXY), CB_ADDSTRING, 0, (LPARAM)buf);
        LoadString(hInst, IDS_ENABLE_PROXY, buf, 256);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_PROXY), CB_ADDSTRING, 0, (LPARAM)buf);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_PROXY), CB_SETCURSEL, _header->proxy.enable ? 1 : 0, NULL);
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), _header->proxy.addr);
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), _header->proxy.user);
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), _header->proxy.pass);
        if (_header->proxy.port == 0)
            SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), _T(""));
        else
            SetDlgItemInt(hDlg, IDC_EDIT_PROXY_PORT, _header->proxy.port, TRUE);
        if (_header->proxy.enable)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), FALSE);
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), buf, 64-1);
            if (buf[0])
            {
                value = GetDlgItemInt(hDlg, IDC_EDIT_PROXY_PORT, &bResult, FALSE);
                if (!bResult || value <= 0 || value >= 0xFFFF)
                {
                    MessageBox_(hDlg, IDS_INVALID_PORT, IDS_ERROR, MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                _header->proxy.port = value;
            }
            else
            {
                _header->proxy.port = 0;
            }

            res = SendMessage(GetDlgItem(hDlg, IDC_COMBO_PROXY), CB_GETCURSEL, 0, NULL);
            if (res == 1)
            {
                _header->proxy.enable = TRUE;
            }
            else
            {
                _header->proxy.enable = FALSE;
            }
            GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), buf, 64-1);
            wcscpy(_header->proxy.addr, buf);
            GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), buf, 64-1);
            wcscpy(_header->proxy.user, buf);
            GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), buf, 64-1);
            wcscpy(_header->proxy.pass, buf);
            Save(GetParent(hDlg));
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_COMBO_PROXY:
            res = SendMessage(GetDlgItem(hDlg, IDC_COMBO_PROXY), CB_GETCURSEL, 0, NULL);
            if (res == 0)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), FALSE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), TRUE);
            }
            break;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK BgImage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT res;
    TCHAR text[MAX_PATH] = {0};
    TCHAR *ext;
    RECT rc;
    switch (message)
    {
    case WM_INITDIALOG:
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_BIENABLE), BM_SETCHECK, _header->bg_image.enable ? BST_CHECKED : BST_UNCHECKED, NULL);
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_BIFILE), _header->bg_image.file_name);
        LoadString(hInst, IDS_STRETCH, text, MAX_PATH);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_ADDSTRING, 0, (LPARAM)text);
        LoadString(hInst, IDS_TILE, text, MAX_PATH);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_ADDSTRING, 1, (LPARAM)text);
        LoadString(hInst, IDS_TILEFLIP, text, MAX_PATH);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_ADDSTRING, 2, (LPARAM)text);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_SETCURSEL, _header->bg_image.mode, NULL);
        if (_header->bg_image.enable)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BIMODE), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BIFILE), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BISEL), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BIMODE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BIFILE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BISEL), FALSE);
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_BIENABLE), BM_GETCHECK, 0, NULL);
            if (res == BST_CHECKED)
            {
                GetWindowText(GetDlgItem(hDlg, IDC_EDIT_BIFILE), text, MAX_PATH-1);
                if (!text[0])
                {
                    MessageBox_(hDlg, IDS_PLS_SEL_IMG, IDS_ERROR, MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                ext = PathFindExtension(text);
                if (_tcscmp(ext, _T(".jpg")) != 0
                    && _tcscmp(ext, _T(".png")) != 0
                    && _tcscmp(ext, _T(".bmp")) != 0
                    && _tcscmp(ext, _T(".jpeg")) != 0)
                {
                    MessageBox_(hDlg, IDS_INVALID_IMG, IDS_ERROR, MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                if (!FileExists(text))
                {
                    MessageBox_(hDlg, IDS_IMG_NOT_EXIST, IDS_ERROR, MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                res = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_GETCURSEL, 0, NULL);
                if (res == -1)
                {
                    MessageBox_(hDlg, IDS_INVALID_LAYOUT, IDS_ERROR, MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                _header->bg_image.mode = res;
                _header->bg_image.enable = 1;
                _tcscpy(_header->bg_image.file_name, text);
                if (_Book)
                    _Book->ReDraw(GetParent(hDlg));
                else
                {
                    GetClientRectExceptStatusBar(GetParent(hDlg), &rc);
                    InvalidateRect(GetParent(hDlg), &rc, FALSE);
                }
            }
            else
            {
                _header->bg_image.enable = 0;
                if (_Book)
                    _Book->ReDraw(GetParent(hDlg));
                else
                {
                    GetClientRectExceptStatusBar(GetParent(hDlg), &rc);
                    InvalidateRect(GetParent(hDlg), &rc, FALSE);
                }
            }
            Save(GetParent(hDlg));
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_CHECK_BIENABLE:
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_BIENABLE), BM_GETCHECK, 0, NULL);
            if (res == BST_CHECKED)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BIMODE), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BIFILE), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BISEL), TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BIMODE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BIFILE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BISEL), FALSE);
            }
            break;
        case IDC_BUTTON_BISEL:
            if (IsWindow(_hFindDlg))
            {
                DestroyWindow(_hFindDlg);
                _hFindDlg = NULL;
            }
            TCHAR szFileName[MAX_PATH] = {0};
            TCHAR szTitle[MAX_PATH] = {0};
            OPENFILENAME ofn = {0};
            ofn.lStructSize = sizeof(ofn);  
            ofn.hwndOwner = hDlg;  
            ofn.lpstrFilter = _T("jpg(*.jpg)\0*.jpg\0jpeg(*.jpeg)\0*.jpeg\0png(*.png)\0*.png\0bmp(*.bmp)\0*.bmp\0\0");
            ofn.lpstrInitialDir = NULL;
            ofn.lpstrFile = szFileName; 
            ofn.nMaxFile = sizeof(szFileName)/sizeof(*szFileName);  
            ofn.nFilterIndex = 0;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
            BOOL bSel = GetOpenFileName(&ofn);
            if (!bSel)
            {
                return 0;
            }
            SetWindowText(GetDlgItem(hDlg, IDC_EDIT_BIFILE), szFileName);
            break;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK JumpProgress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hWnd;
    TCHAR buf[16] = {0};
    double progress;

    switch (message)
    {
    case WM_INITDIALOG:
        hWnd = GetDlgItem(hDlg, IDC_EDIT_JP);
        if (_Book)
        {
            progress = _Book->GetProgress();
            _stprintf(buf, _T("%0.2f"), progress);
            SetWindowText(hWnd, buf);
        }
        SetFocus(hWnd);
        SendMessage(hWnd, EM_SETSEL, 0, -1);
        return (INT_PTR)FALSE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                if (_item)
                {
                    hWnd = GetDlgItem(hDlg, IDC_EDIT_JP);
                    GetWindowText(hWnd, buf, 15);
                    progress = _tstof(buf);
                    if (progress < 0.0 || progress > 100.0)
                    {
                        MessageBox_(hDlg, IDS_INVALID_VALUE, IDS_ERROR, MB_OK|MB_ICONERROR);
                        return (INT_PTR)TRUE;
                    }
                    if (_Book)
                    {
                        _item->index = (int)(progress * _Book->GetTextLength() / 100);
                        if (_item->index == _Book->GetTextLength())
                            _item->index--;
                        _Book->Reset(GetParent(hDlg));
                        Save(GetParent(hDlg));
                    }
                }
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

#if ENABLE_NETWORK
INT_PTR CALLBACK UpgradeProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    json_item_data_t *vinfo;
    switch (message)
    {
    case WM_INITDIALOG:
        vinfo = _Upgrade.GetVersionInfo();
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_UPGRADE_VERSION), vinfo->version.c_str());
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_UPGRADE_DESC), vinfo->desc.c_str());
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDC_BUTTON_UPGRADE_INGORE:
            vinfo = _Upgrade.GetVersionInfo();
            wcscpy(_header->ingore_version, vinfo->version.c_str());
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_BUTTON_UPGRADE_DOWN:
            vinfo = _Upgrade.GetVersionInfo();
            ShellExecute(NULL, _T("open"), vinfo->url.c_str(), NULL, NULL, SW_SHOWNORMAL);
            break;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}
#endif

LRESULT OnCreate(HWND hWnd)
{
    _WndInfo.hMenu = GetMenu(hWnd);
    // create status bar
    _WndInfo.hStatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE, _T("Please open a text."), hWnd, IDC_STATUSBAR);
    // register find text dialog
    _uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);
    // create chapters tree view
    _hTreeView = CreateWindow(WC_TREEVIEW, _T("Tree View"), 
        /*WS_VISIBLE | */WS_CHILD /*| WS_BORDER*/ | TVS_HASLINES | TVS_NOHSCROLL | TVS_NOTOOLTIPS | TVS_LINESATROOT,
        0, 0, 200, 300, hWnd, NULL, hInst, NULL);
    _hTreeMark = CreateWindow(WC_TREEVIEW, _T("Tree Mark"), 
        /*WS_VISIBLE | */WS_CHILD /*| WS_BORDER*/ | TVS_HASLINES | TVS_NOHSCROLL /*| TVS_NOTOOLTIPS*/ | TVS_LINESATROOT,
        0, 0, 200, 300, hWnd, NULL, hInst, NULL);
    //TreeView_SetBkColor(_hTreeView, GetSysColor(COLOR_MENU));
    //TreeView_SetBkColor(_hTreeMark, GetSysColor(COLOR_MENU));
    SetTreeviewFont();

    RemoveMenus(hWnd, FALSE);
    OnUpdateMenu(hWnd);

    // open the last file
    if (_header->item_count > 0)
    {
        item_t* item = _Cache.get_item(0);
        OnOpenBook(hWnd, item->file_name, FALSE);
    }

    // check upgrade
#if ENABLE_NETWORK
    CheckUpgrade(hWnd);
    StartCheckBookUpdate(hWnd);
#endif

    // init keyset
    KS_Init(hWnd, _header->keyset);

    return 0;
}

LRESULT OnUpdateMenu(HWND hWnd)
{
    static Bitmap* s_bitmap = NULL;
    static HGLOBAL s_hMemory = NULL;
    static HBITMAP hBitmap = NULL;
    TCHAR buf[MAX_LOADSTRING];
    int menu_begin_id = IDM_OPEN_BEGIN;
    HMENU hMenuBar = GetMenu(hWnd);
    HMENU hFile = GetSubMenu(hMenuBar, 0);
    MENUITEMINFO mi = { 0 };

    if (hFile)
    {
        DeleteMenu(hMenuBar, 0, MF_BYPOSITION);
        hFile = NULL;
    }
    if (hBitmap)
    {
        DeleteObject(hBitmap);
        hBitmap = NULL;
    }
    if (!s_bitmap)
    {
        LoadResourceImage(MAKEINTRESOURCE(IDB_PNG_NEW), _T("PNG"), &s_bitmap, &s_hMemory);
    }
    if (s_bitmap)
    {
        s_bitmap->GetHBITMAP(Color(0, 0, 0, 0), &hBitmap);
    }
    hFile = CreateMenu();
    LoadString(hInst, IDS_MENU_OPEN, buf, MAX_LOADSTRING);
    AppendMenu(hFile, MF_STRING, IDM_OPEN, buf);
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    for (int i=0; i<_header->item_count; i++)
    {
        item_t* item = _Cache.get_item(i);
        AppendMenu(hFile, MF_STRING, (UINT_PTR)menu_begin_id, item->file_name);
        if (0 == _tcscmp(PathFindExtension(item->file_name), _T(".ol")) && item->is_new)
        {
            SetMenuItemBitmaps(hFile, menu_begin_id, MF_BITMAP, hBitmap, hBitmap);
        }
        menu_begin_id++;
    }
    if (_header->item_count > 0)
        AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    LoadString(hInst, IDS_MENU_CLEAR, buf, MAX_LOADSTRING);
    AppendMenu(hFile, MF_STRING, IDM_CLEAR, buf);
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    LoadString(hInst, IDS_MENU_EXIT, buf, MAX_LOADSTRING);
    AppendMenu(hFile, MF_STRING, IDM_EXIT, buf);
    LoadString(hInst, IDS_MENU_FILE, buf, MAX_LOADSTRING);
    InsertMenu(hMenuBar, 0, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hFile, buf);
    DrawMenuBar(hWnd);

    return 0;
}

LRESULT OnOpenItem(HWND hWnd, int item_id, BOOL forced)
{
    if (IsWindow(_hFindDlg))
    {
        DestroyWindow(_hFindDlg);
        _hFindDlg = NULL;
    }
    if (!forced && _item && _item->id == item_id && _Book && !_Book->IsLoading())
    {
        return 0;
    }

    BOOL bNotExist = FALSE;
    item_t* item = _Cache.get_item(item_id);
    if (!item)
    {
        bNotExist = TRUE;
    }
    else
    {
        if (PathFileExists(item->file_name))
        {
            OnOpenBook(hWnd, item->file_name, forced);
        }
        else
        {
            bNotExist = TRUE;
        }
    }

    if (bNotExist)
    {
        _Cache.delete_item(item_id);
        // update menu
        OnUpdateMenu(hWnd);
        return 0L;
    }

    return 0;
}

LRESULT OnClearFileList(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    if (_Book)
    {
        delete _Book;
        _Book = NULL;
    }
    _Cache.delete_all_item();
    OnUpdateMenu(hWnd);
    PostMessage(hWnd, WM_UPDATE_CHAPTERS, 0, NULL);
    GetClientRectExceptStatusBar(hWnd, &rect);
    InvalidateRect(hWnd, &rect, FALSE);
    SetWindowText(hWnd, szTitle);
    StopLoadingImage(hWnd);
    StopAutoPage(hWnd);
    Save(hWnd);
    return 0;
}

LRESULT OnSetFont(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CHOOSEFONT cf;            // common dialog box structure
    LOGFONT logFont;
    static DWORD rgbCurrent;   // current text color
    BOOL bUpdate = FALSE;

    // Initialize CHOOSEFONT
    ZeroMemory(&cf, sizeof(cf));
    memcpy(&logFont, &_header->font, sizeof(LOGFONT));
    cf.lStructSize = sizeof (cf);
    cf.hwndOwner = hWnd;
    cf.lpLogFont = &logFont;
    cf.rgbColors = _header->font_color;
    cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS;

    if (ChooseFont(&cf))
    {
        if (_header->font_color != cf.rgbColors)
        {
            _header->font_color = cf.rgbColors;
            bUpdate = TRUE;
        }
        _header->font.lfQuality = PROOF_QUALITY;

        if (0 != memcmp(&logFont, &_header->font, sizeof(LOGFONT)))
        {
            memcpy(&_header->font, &logFont, sizeof(LOGFONT));
            bUpdate = TRUE;
        }
        if (bUpdate && _Book)
        {
            SetTreeviewFont();
            _Book->Reset(hWnd);
            Save(hWnd);
        }
    }

    return 0;
}

LRESULT OnSetBkColor(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    CHOOSECOLOR cc;                 // common dialog box structure 
    static COLORREF acrCustClr[16]; // array of custom colors 

    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hWnd;
    cc.lpCustColors = (LPDWORD) acrCustClr;
    cc.rgbResult = _header->bg_color;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc))
    {
        if (_header->bg_color != cc.rgbResult)
        {
            _header->bg_color = cc.rgbResult;
            //_Book->ReDraw(hWnd);
            GetClientRectExceptStatusBar(hWnd, &rect);
            InvalidateRect(hWnd, &rect, FALSE);
            Save(hWnd);
        }
    }
    
    return 0;
}

LRESULT OnRestoreDefault(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int lastrule = _header->chapter_rule.rule;
    BYTE lastmenufont = _header->meun_font_follow;

    if (IDNO == MessageBox_(hWnd, IDS_RESTORE_DEFAULT, IDS_WARN, MB_ICONINFORMATION|MB_YESNO))
    {
        return 0;
    }

    if (IsZoomed(hWnd))
        SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    KS_UnRegisterAllHotKey(hWnd);
    _Cache.default_header();
    UpdateLayoutForDpi(&_header->rect);
    UpdateFontForDpi(&_header->font);
#if 0 // needn't
#if ENABLE_TAG
    for (int i = 0; i < MAX_TAG_COUNT; i++)
    {
        UpdateFontForDpi(hWnd, &_header->tags[i].font);
    }
#endif
#endif
    KS_RegisterAllHotKey(hWnd);
    ShowSysTray(hWnd, _header->show_systray);
    ShowInTaskbar(hWnd, !_header->hide_taskbar);

    // change window size
    SetWindowPos(hWnd, NULL,
        _header->rect.left, _header->rect.top,
        _header->rect.right - _header->rect.left,
        _header->rect.bottom - _header->rect.top,
        /*SWP_DRAWFRAME*/SWP_NOREDRAW);

    if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
    {
        ResetLayerd(hWnd);
        OnDraw(hWnd);
    }
    else
    {
        ResetLayerd(hWnd);
        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
        InvalidateRect(hWnd, NULL, TRUE);
    }

    if (lastrule != _header->chapter_rule.rule)
    {
        // reload book
        OnOpenItem(hWnd, 0, TRUE);
    }
    else
    {
        if (_Book)
            _Book->Reset(hWnd);
    }
    if (lastmenufont != _header->meun_font_follow)
        SetTreeviewFont();

    Save(hWnd);
    return 0;
}

LRESULT OnPaint(HWND hWnd, HDC hdc)
{
    RECT rc;
    HBRUSH hBrush = NULL;
    HDC memdc = NULL;
    HBITMAP hBmp = NULL;
    HFONT hFont = NULL;
    Bitmap *image;
    Graphics *g = NULL;
    Rect rect;
    UINT w,h;
    double scale;

    GetClientRectExceptStatusBar(hWnd, &rc);

    // memory dc
    memdc = CreateCompatibleDC(hdc);

    // load bg image
    image = LoadBGImage(rc.right-rc.left,rc.bottom-rc.top);
    if (image)
    {
        image->GetHBITMAP(Color(0, 0, 0, 0), &hBmp);
        SelectObject(memdc, hBmp);
    }
    else
    {
        hBmp = CreateCompatibleBitmap(hdc, rc.right-rc.left, rc.bottom-rc.top);
        SelectObject(memdc, hBmp);

        // set bg color
        hBrush = CreateSolidBrush(_header->bg_color);
        SelectObject(memdc, hBrush);
        FillRect(memdc, &rc, hBrush);
    }

    // set font
    hFont = CreateFontIndirect(&_header->font);
    SelectObject(memdc, hFont);
    SetTextColor(memdc, _header->font_color);
    
    SetBkMode(memdc, TRANSPARENT);

    if (_Book && !_Book->IsLoading() && _bShowText)
        _Book->DrawPage(hWnd, memdc);
    if (_loading && _loading->enable && _bShowText)
    {
        g = new Graphics(memdc);
        w = (UINT)(rc.right - rc.left) > _loading->image->GetWidth() ? _loading->image->GetWidth() : (UINT)(rc.right - rc.left);
        h = (UINT)(rc.bottom - rc.top) > _loading->image->GetHeight() ? _loading->image->GetHeight() : (UINT)(rc.bottom - rc.top);
        scale = ((double)_loading->image->GetWidth())/_loading->image->GetHeight();
        if (((double)w)/h > scale)
        {
            // image is too high
            w = (int)(scale * h);
        }
        else
        {
            // image is too wide
            h = (int)(w / scale);
        }

        rect.X = (rc.right - rc.left - w)/2;
        rect.Y = (rc.bottom - rc.top - h)/2;
        rect.Width = w;
        rect.Height = h;
        g->SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g->DrawImage(_loading->image, rect, 0, 0, _loading->image->GetWidth(), _loading->image->GetHeight(), UnitPixel);
    }

    BitBlt(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, memdc, rc.left, rc.top, SRCCOPY);

    DeleteObject(hBmp);
    DeleteObject(hFont);
    DeleteObject(hBrush);
    DeleteDC(memdc);
    if (g)
        delete g;
    UpdateProgess();
    UpdateTitle(hWnd);
    return 0;
}

void OnDraw(HWND hWnd)
{
    RECT rc;
    HBRUSH hBrush = NULL;
    HDC memdc = NULL;
    HBITMAP hBmp = NULL;
    HFONT hFont = NULL;
    Bitmap *image;
    Graphics *g = NULL;
    Rect rect;
    INT w,h;
    double scale;
    HDC hdcScreen = NULL;
    HBITMAP hTextBmp = NULL;
    static u32 s_bgColor = 0;
    static Bitmap *s_bgColorImage = NULL;
    static int s_width = 0;
    static int s_height = 0;

    GetClientRectExceptStatusBar(hWnd, &rc);

    w = rc.right-rc.left;
    h = rc.bottom-rc.top;

#if 1
    // load bg image
    image = LoadBGImage(w, h, _header->alpha);
    if (!image)
    {
        if (s_bgColorImage && s_width == w && s_height == h && s_bgColor == _header->bg_color)
        {
            image = s_bgColorImage;
        }
        else
        {
            if (s_bgColorImage)
                delete s_bgColorImage;
            s_bgColorImage = new Bitmap(w, h, PixelFormat32bppARGB);
            // set bg color
            Gdiplus::Color color;
            Gdiplus::ARGB argb;
            Gdiplus::Graphics *g = Gdiplus::Graphics::FromImage(s_bgColorImage);
            color.SetFromCOLORREF(_header->bg_color);
            argb = color.GetValue();
            argb &= 0x00FFFFFF;
            argb |= (((DWORD)_header->alpha) << 24);
            color.SetValue(argb);
            Gdiplus::SolidBrush brush_tr(color);
            g->FillRectangle(&brush_tr, 0, 0, w, h);
            delete g;
            image = s_bgColorImage;
        }
    }

    // memory dc
    hdcScreen = GetDC(NULL);
    memdc = CreateCompatibleDC(hdcScreen);
    image->GetHBITMAP(Color(0, 0, 0, 0), &hBmp);
    SelectObject(memdc, hBmp);
#else
    // memory dc
    hdcScreen = GetDC(NULL);
    memdc = CreateCompatibleDC(hdcScreen);

    // load bg image
    image = LoadBGImage(w, h, _header->alpha);
    if (image)
    {
        image->GetHBITMAP(Color(0, 0, 0, 0), &hBmp);
        SelectObject(memdc, hBmp);
    }
    else
    {
        hBmp = CreateCompatibleBitmap(hdcScreen, rc.right-rc.left, rc.bottom-rc.top);
        SelectObject(memdc, hBmp);

        // set bg color
        COLORREF color = _header->bg_color;
        hBrush = CreateSolidBrush(_header->bg_color);
        color &= 0x00FFFFFF;
        color |= (((DWORD)_header->alpha) << 24);
        SelectObject(memdc, hBrush);
        FillRect(memdc, &rc, hBrush);
    }
#endif

#if 0  // move to LoadBGImage() and Brush color
    // set bitmap alpha
    Gdiplus::Color color;
    Gdiplus::ARGB argb;
    for (INT i=0; i<h; i++)
    {
        for (INT j=0; j<w; j++)
        {
            image->GetPixel(j, i, &color);
            argb = color.GetValue();
            argb &= 0x00FFFFFF;
            argb |= (((DWORD)_header->alpha) << 24);
            color.SetValue(argb);
            image->SetPixel(j, i, color);
        }
    }
#endif

    // draw text
    if (_Book && !_Book->IsLoading() && _bShowText)
    {
        hFont = CreateFontIndirect(&_header->font);
        hTextBmp = CreateAlphaTextBitmap(hWnd, hFont, _header->font_color, w, h);
    }
    if (_loading && _loading->enable && _bShowText)
    {
        g = new Graphics(memdc);
        w = (UINT)(rc.right - rc.left) > _loading->image->GetWidth() ? _loading->image->GetWidth() : (UINT)(rc.right - rc.left);
        h = (UINT)(rc.bottom - rc.top) > _loading->image->GetHeight() ? _loading->image->GetHeight() : (UINT)(rc.bottom - rc.top);
        scale = ((double)_loading->image->GetWidth())/_loading->image->GetHeight();
        if (((double)w)/h > scale)
        {
            // image is too high
            w = (int)(scale * h);
        }
        else
        {
            // image is too wide
            h = (int)(w / scale);
        }

        rect.X = (rc.right - rc.left - w)/2;
        rect.Y = (rc.bottom - rc.top - h)/2;
        rect.Width = w;
        rect.Height = h;
        g->SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g->DrawImage(_loading->image, rect, 0, 0, _loading->image->GetWidth(), _loading->image->GetHeight(), UnitPixel);
    }

    // alpha blend text to backgroud image
    if (hTextBmp)
    {
        HDC hTempDC = CreateCompatibleDC(hdcScreen);
        HBITMAP hOldBMP = (HBITMAP)SelectObject(hTempDC, hTextBmp);
        if (hOldBMP)
        {
#if 1
            // Fill blend function and blend new text to window 
            BLENDFUNCTION bf;
            bf.BlendOp = AC_SRC_OVER;
            bf.BlendFlags = 0;
            bf.SourceConstantAlpha = 0xFF; 
            bf.AlphaFormat = AC_SRC_ALPHA;
            AlphaBlend(memdc, 0, 0, w, h, hTempDC, 0, 0, w, h, bf);
#else
            BitBlt(memdc, 0, 0, w, h, hTempDC, 0, 0, SRCCOPY);
#endif

            // Clean up 
            SelectObject(hTempDC, hOldBMP); 
            DeleteObject(hTextBmp); 
            DeleteDC(hTempDC); 
        }
    }

    // update layered
    RECT winRect;
    GetWindowRect(hWnd, &winRect);
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 0xFF;
    blend.AlphaFormat = AC_SRC_ALPHA;
    POINT ptPos = {winRect.left, winRect.top};
    SIZE sizeWnd = {winRect.right - winRect.left, winRect.bottom - winRect.top};
    POINT ptSrc = {0, 0};
    BOOL ret = UpdateLayeredWindow(hWnd, hdcScreen, &ptPos, &sizeWnd, memdc, &ptSrc, 0, &blend, ULW_ALPHA);
    

    DeleteObject(hBmp);
    DeleteObject(hBrush);
    DeleteObject(hFont);
    DeleteDC(memdc);
    ReleaseDC(NULL, hdcScreen);
    if (g)
        delete g;
    UpdateProgess();
    UpdateTitle(hWnd);
    return;
}

void ResetLayerd(HWND hWnd)
{
    static BOOL s_bHideBorder = FALSE;
    static BOOL s_bFullScreen = FALSE;

    if (s_bHideBorder != _WndInfo.bHideBorder || s_bFullScreen != _WndInfo.bFullScreen)
    {
        DWORD exstyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        exstyle &= ~WS_EX_LAYERED;
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, exstyle);
        exstyle |= WS_EX_LAYERED;
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, exstyle);
        s_bHideBorder = _WndInfo.bHideBorder;
        s_bFullScreen = _WndInfo.bFullScreen;
    }
}

LRESULT OnSize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc;

    GetClientRectExceptStatusBar(hWnd, &rc);
    if (_Book)
        _Book->SetRect(&rc);
    SendMessage(_WndInfo.hStatusBar, message, wParam, lParam);
    return 0;
}

LRESULT OnMove(HWND hWnd)
{
    // save rect to cache file when exit app
    return 0;
}

LRESULT OnHideWin(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ShowHideWindow(hWnd);
    return 0;
}

LRESULT OnHideBorder(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    Book *pBook = NULL;

    if (_WndInfo.bFullScreen)
        return 0;

    GetClientRectExceptStatusBar(hWnd, &rc);
    ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.left));
    ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.right));

    // save status
    if (!_WndInfo.bHideBorder)
    {
        _WndInfo.hbStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
        _WndInfo.hbExStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        GetWindowRect(hWnd, &_WndInfo.hbRect);
        _WndInfo.hbRect.left = rc.left - _WndInfo.hbRect.left;
        _WndInfo.hbRect.right = _WndInfo.hbRect.right - rc.right;
        _WndInfo.hbRect.top = rc.top - _WndInfo.hbRect.top;
        _WndInfo.hbRect.bottom = _WndInfo.hbRect.bottom - rc.bottom;
    }
    else
    {
        rc.left = rc.left - _WndInfo.hbRect.left;
        rc.right = rc.right + _WndInfo.hbRect.right;
        rc.top = rc.top - _WndInfo.hbRect.top;
        rc.bottom = rc.bottom + _WndInfo.hbRect.bottom;
    }

    // set new status
    _WndInfo.bHideBorder = !_WndInfo.bHideBorder;
    pBook = _Book;_Book = NULL; // lock onsize
    // hide border
    if (_WndInfo.bHideBorder)
    {
        SetWindowLongPtr(hWnd, GWL_STYLE, _WndInfo.hbStyle & (~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU)));
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, _WndInfo.hbExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
        SetMenu(hWnd, NULL);
        ShowWindow(_WndInfo.hStatusBar, SW_HIDE);
        SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, /*SWP_DRAWFRAME*/SWP_NOREDRAW);
    }
    // show border
    else
    {
        SetWindowLongPtr(hWnd, GWL_STYLE, _WndInfo.hbStyle);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, _WndInfo.hbExStyle);
        SetMenu(hWnd, _WndInfo.hMenu);
        ShowWindow(_WndInfo.hStatusBar, SW_SHOW);
        SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, /*SWP_DRAWFRAME*/SWP_NOREDRAW);
    }

    _Book = pBook;

    // for alpha
    if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
    {
        ResetLayerd(hWnd);
        OnDraw(hWnd);
    }
    else
    {
        ResetLayerd(hWnd);
        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
        InvalidateRect(hWnd, NULL, TRUE);
    }
    return 0;
}

LRESULT OnFullScreen(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MONITORINFO mi;
    RECT rc;

    if (!_WndInfo.bFullScreen)
    {
        _WndInfo.fsStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
        _WndInfo.fsExStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        GetWindowRect(hWnd, &_WndInfo.fsRect);
    }

    _WndInfo.bFullScreen = !_WndInfo.bFullScreen;

    if (_WndInfo.bFullScreen)
    {
        SetWindowLongPtr(hWnd, GWL_STYLE, _WndInfo.fsStyle & (~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU)));
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, _WndInfo.fsExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
        SetMenu(hWnd, NULL);
        ShowWindow(_WndInfo.hStatusBar, SW_HIDE);


        mi.cbSize = sizeof(mi);
        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST),&mi);
        rc = mi.rcMonitor;
        SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_DRAWFRAME);
    }
    else
    {
        SetWindowLongPtr(hWnd, GWL_STYLE, _WndInfo.fsStyle);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, _WndInfo.fsExStyle);
        if (!_WndInfo.bHideBorder)
        {
            SetMenu(hWnd, _WndInfo.hMenu);
            ShowWindow(_WndInfo.hStatusBar, SW_SHOW);
        }
        SetWindowPos(hWnd, NULL, _WndInfo.fsRect.left, _WndInfo.fsRect.top, 
            _WndInfo.fsRect.right-_WndInfo.fsRect.left, _WndInfo.fsRect.bottom-_WndInfo.fsRect.top, SWP_DRAWFRAME);
    }

    // for alpha
    if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
    {
        ResetLayerd(hWnd);
        OnDraw(hWnd);
    }
    else
    {
        ResetLayerd(hWnd);
        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
        InvalidateRect(hWnd, NULL, TRUE);
    }
    return 0;
}

LRESULT OnTopmost(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool isTopmost = false;
    SetWindowPos(hWnd, isTopmost ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    isTopmost = !isTopmost;
    return 0;
}

LRESULT OnOpenFile(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (IsWindow(_hFindDlg))
    {
        DestroyWindow(_hFindDlg);
        _hFindDlg = NULL;
    }
    TCHAR szFileName[MAX_PATH] = {0};
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);  
    ofn.hwndOwner = hWnd;  
    ofn.lpstrFilter = _T("Files (*.txt;*.epub;*.ol)\0*.txt;*.epub;*.ol\0\0");
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrFile = szFileName; 
    ofn.nMaxFile = sizeof(szFileName)/sizeof(*szFileName);  
    ofn.nFilterIndex = 0;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    BOOL bSel = GetOpenFileName(&ofn);
    if (!bSel)
    {
        return 0;
    }

    OnOpenBook(hWnd, szFileName, FALSE);
    return 0;
}

LRESULT OnAddMark(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int ret;
    if (_Book && !_Book->IsLoading() && _Book->GetBookType() != book_online)
    {
        ret = MessageBox_(hWnd, IDS_ADD_BOOKMARK_TIPS, IDS_BOOKMARK, MB_ICONINFORMATION|MB_YESNO);
        if (ret == IDYES)
        {
            if (_Cache.add_mark(_item, _item->index))
            {
                OnUpdateBookMark(hWnd);
                Save(hWnd);
            }
        }
    }
    return 0;
}

LRESULT OnAutoPage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        if (_IsAutoPage)
        {
            StopAutoPage(hWnd);
        }
        else
        {
            StartAutoPage(hWnd);
        }
    }
    return 0;
}

LRESULT OnSearch(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        OnFindText(hWnd, message, wParam, lParam);
    return 0;
}

LRESULT OnJump(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        DialogBox(hInst, MAKEINTRESOURCE(IDD_JUMP_PROGRESS), hWnd, JumpProgress);
    return 0;
}

int Editwordbreakproc(
    LPTSTR lpch,
    int ichCurrent,
    int cch,
    int code
    )
{
    if (WB_ISDELIMITER == code && cch == 0x0a)
    {
        return TRUE;
    }
    return FALSE;
}

LRESULT OnEditMode(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR *text = NULL;
    BOOL readonly = TRUE;

    if (EC_IsEditMode())
    {
        EC_LeaveEditMode();
        return 0;
    }

    if (_Book && !_Book->IsLoading())
    {
        if (_Book->GetCurPageText(&text))
        {
            if (_WndInfo.bHideBorder)
            {
                OnHideBorder(hWnd, message, wParam, lParam);
            }

            if (_Book->GetBookType() == book_text)
            {
                readonly = FALSE;
            }

            EC_EnterEditMode(hInst, hWnd, &_header->font, text, readonly);

            free(text);
        }
    }

    return 0;
}

LRESULT OnPageUp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->PageUp(hWnd);
        _NeedSave = TRUE;
    }
    return 0;
}

LRESULT OnPageDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->PageDown(hWnd);
        _NeedSave = TRUE;
    }
    return 0;
}

LRESULT OnLineUp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->LineUp(hWnd, _header->wheel_speed);
        _NeedSave = TRUE;
    }
    return 0;
}

LRESULT OnLineDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->LineDown(hWnd, _header->wheel_speed);
        _NeedSave = TRUE;
    }
    return 0;
}

LRESULT OnChapterUp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->JumpPrevChapter(hWnd);
        Save(hWnd);
    }
    return 0;
}

LRESULT OnChapterDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        _Book->JumpNextChapter(hWnd);
        Save(hWnd);
    }
    return 0;
}

LRESULT OnFontZoomIn(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    const int ZOOM_MIN = 1;
    const int ZOOM_MAX = 96;
    const int ZOOM_STEP = 1;

    if (_header && _Book && !_Book->IsLoading())
    {
        if (abs(_header->font.lfHeight) < ZOOM_MAX)
        {
            if (_header->font.lfHeight > 0)
                _header->font.lfHeight++;
            else
                _header->font.lfHeight--;
            _Book->Reset(hWnd);
            Save(hWnd);
        }
    }
    return 0;
}

LRESULT OnFontZoomOut(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    const int ZOOM_MIN = 1;
    const int ZOOM_MAX = 96;
    const int ZOOM_STEP = 1;

    if (_header && _Book && !_Book->IsLoading())
    {
        if (abs(_header->font.lfHeight) > ZOOM_MIN)
        {
            if (_header->font.lfHeight > 0)
                _header->font.lfHeight--;
            else
                _header->font.lfHeight++;
            _Book->Reset(hWnd);
            Save(hWnd);
        }
    }
    return 0;
}

LRESULT OnDropFiles(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDROP hDropInfo = (HDROP)wParam;
    UINT  nFileCount = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
    TCHAR szFileName[MAX_PATH] = _T("");
    DWORD dwAttribute;
    TCHAR *ext = NULL;

    for (UINT i = 0; i < nFileCount; i++)
    {
        ::DragQueryFile(hDropInfo, i, szFileName, sizeof(szFileName));
        dwAttribute = ::GetFileAttributes(szFileName);

        if (dwAttribute & FILE_ATTRIBUTE_DIRECTORY)
        {          
            continue;
        }

        // check is txt file
        ext = PathFindExtension(szFileName);
        if (ext && _tcscmp(ext, _T(".txt")) && _tcscmp(ext, _T(".epub")) && _tcscmp(ext, _T(".ol")))
        {
            continue;
        }

        // open file
        OnOpenBook(hWnd, szFileName, FALSE);

        // just open first file
        break;
    }

    ::DragFinish(hDropInfo);
    return 0;
}

LRESULT OnFindText(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static FINDREPLACE fr;       // common dialog box structure
    static TCHAR szFindWhat[80] = {0}; // buffer receiving string

    if (message == _uFindReplaceMsg)
    {
        // do search
        if (!_Book)
            return 0;
        int len = _tcslen(szFindWhat);
        if (fr.Flags & FR_DIALOGTERM)
        {
            // close dlg
            DestroyWindow(_hFindDlg);
            _hFindDlg = NULL;
        }
        else if (fr.Flags & FR_DOWN) // back search
        {
            for (int i=_item->index+1; i<_Book->GetTextLength()-len+1; i++)
            {
                if (0 == memcmp(szFindWhat, _Book->GetText()+i, len*sizeof(TCHAR)))
                {
                    _item->index = i;
                    _Book->Reset(hWnd);
                    Save(hWnd);
                    break;
                }
            }
        }
        else // front search
        {
            for (int i=_item->index-1; i>=0; i--)
            {
                if (0 == memcmp(szFindWhat, _Book->GetText()+i, len*sizeof(TCHAR)))
                {
                    _item->index = i;
                    _Book->Reset(hWnd);
                    Save(hWnd);
                    break;
                }
            }
        }
    }
    else
    {
        if (!IsWindow(_hFindDlg))
        {
            // Initialize FINDREPLACE
            ZeroMemory(&fr, sizeof(fr));
            fr.lStructSize = sizeof(fr);
            fr.hwndOwner = hWnd;
            fr.hInstance = hInst;
            fr.lpstrFindWhat = szFindWhat;
            fr.wFindWhatLen = 80;
            fr.Flags = FR_DOWN;

            _hFindDlg = FindText(&fr);
        }
    }

    return 0;
}

LRESULT OnUpdateChapters(HWND hWnd)
{
    chapters_t *chapters;
    chapters_t::iterator itor;

    TVITEM tvi = {0};
    TVINSERTSTRUCT tvins = {0};
    HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
    
    TreeView_DeleteAllItems(_hTreeView);
    if (_Book)
    {
        tvi.mask = TVIF_TEXT /*| TVIF_IMAGE | TVIF_SELECTEDIMAGE */| TVIF_PARAM;

        chapters = _Book->GetChapters();
        for (itor = chapters->begin(); itor != chapters->end(); itor++)
        {
            tvi.pszText = (TCHAR *)itor->second.title.c_str(); 
            tvi.cchTextMax = sizeof(tvi.pszText)/sizeof(tvi.pszText[0]);
            tvi.lParam = (LPARAM)itor->first; 
            tvins.item = tvi; 
            tvins.hInsertAfter = hPrev;
            tvins.hParent = TVI_ROOT;

            // Add the item to the tree-view control. 
            hPrev = (HTREEITEM)SendMessage(_hTreeView, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);
        }
    }

    return 0;
}

LRESULT OnUpdateBookMark(HWND hWnd)
{
    const int MAX_MARK_TEXT = 256;
    TVITEM tvi = {0};
    TVINSERTSTRUCT tvins = {0};
    HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
    int i;
    TCHAR szText[MAX_MARK_TEXT] = {0};
    int len;

    TreeView_DeleteAllItems(_hTreeMark);
    if (_Book && !_Book->IsLoading() && _item)
    {
        tvi.mask = TVIF_TEXT /*| TVIF_IMAGE | TVIF_SELECTEDIMAGE */| TVIF_PARAM;

        for (i=0; i<_item->mark_size; i++)
        {
            len = _item->mark[i] + (MAX_MARK_TEXT - 1) > _Book->GetTextLength() ? _Book->GetTextLength() - _item->mark[i] : (MAX_MARK_TEXT - 1);
            memcpy(szText, _Book->GetText()+_item->mark[i], sizeof(TCHAR)*len);
            szText[len] = 0;

            tvi.pszText = szText; 
            tvi.cchTextMax = MAX_MARK_TEXT;
            tvi.lParam = (LPARAM)i; 
            tvins.item = tvi; 
            tvins.hInsertAfter = hPrev;
            tvins.hParent = TVI_ROOT;

            // Add the item to the tree-view control. 

            hPrev = (HTREEITEM)SendMessage(_hTreeMark, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);
        }
    }

    return 0;
}

LRESULT OnOpenBookResult(HWND hWnd, BOOL result)
{
    item_t *item = NULL;
    RECT rect;
    UINT type;
    TCHAR fileName[MAX_PATH] = { 0 };

    StopLoadingImage(hWnd);
    //EnableWindow(hWnd, TRUE);
    if (!result)
    {
        _tcscpy(fileName, _Book->GetFileName());
        type = _Book->GetBookType() == book_online ? MB_RETRYCANCEL : MB_OK;
        delete _Book;
        _Book = NULL;
        if (IDRETRY == MessageBox_(hWnd, IDS_OPEN_FILE_FAILED, IDS_ERROR, type | MB_ICONERROR))
        {
            OnOpenBook(hWnd, fileName, FALSE);
        }
        return FALSE;
    }

    // create new item
#if ENABLE_MD5
    item = _Cache.find_item(_Book->GetMd5(), _Book->GetFileName());
#else
    item = _Cache.find_item(_Book->GetFileName());
#endif
    if (NULL == item)
    {
#if ENABLE_MD5
        item = _Cache.new_item(_Book->GetMd5(), _Book->GetFileName());
#else
        item = _Cache.new_item(_Book->GetFileName());
#endif
        _header = _Cache.get_header(); // after realloc the header address has been changed
    }

    // open item
    _item = _Cache.open_item(item->id);

    // set param
    GetClientRectExceptStatusBar(hWnd, &rect);
#if ENABLE_TAG
    _Book->Setting(hWnd, &_item->index, &_header->line_gap, &_header->word_wrap, &(_header->internal_border), _header->tags);
#else
    _Book->Setting(hWnd, &_item->index, &_header->line_gap, &_header->word_wrap, &(_header->internal_border));
#endif
    _Book->SetRect(&rect);
    _Book->SetChapterRule(&(_header->chapter_rule));
    if (_Book->GetBookType() == book_online)
    {
        _item->is_new = FALSE;
        ((OnlineBook*)_Book)->UpdateBookSource();
    }

    // update menu
    OnUpdateMenu(hWnd);

    // Update title
    UpdateTitle(hWnd);

    // update chapter
    PostMessage(hWnd, WM_UPDATE_CHAPTERS, 0, NULL);

    // repaint
    _Book->ReDraw(hWnd);

    // save
    Save(hWnd);

    return 0;
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    LPMOUSEHOOKSTRUCT mh = NULL;
    RECT rect;

    if (nCode == HC_ACTION)
    {
        switch (wParam)
        {
        case WM_MOUSEMOVE:
            mh = (LPMOUSEHOOKSTRUCT)lParam;
            GetWindowRect(_hWnd, &rect);
            if (PtInRect(&rect, mh->pt))
            {
                if (!IsWindowVisible(_hWnd))
                    ShowWindow(_hWnd, SW_SHOW);
            }
            else
            {
                if (IsWindowVisible(_hWnd))
                    ShowWindow(_hWnd, SW_HIDE);
            }
            break;
        default:
            break;
        }
    }
    
    return CallNextHookEx(_hMouseHook, nCode, wParam, lParam);
}

void ShowHideWindow(HWND hWnd)
{
    // show or hide window
    BOOL isShow = IsWindowVisible(hWnd);
    ShowWindow(hWnd, isShow ? SW_HIDE : SW_SHOW);
    if (!isShow)
    {
        if (IsIconic(hWnd))
            ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
    }
}

void OnOpenBook(HWND hWnd, TCHAR *filename, BOOL forced)
{
    item_t *item = NULL;
    TCHAR *ext = NULL;
#if ENABLE_MD5
    u128_t md5;
    char *data = NULL;
#else
    FILE *fp = NULL;
#endif
    int size = 0;
    TCHAR szFileName[MAX_PATH] = {0};
    chkbook_arg_t* arg = NULL;

    _tcscpy(szFileName, filename);
    ext = PathFindExtension(szFileName);
    if (_tcscmp(ext, _T(".txt")) && _tcscmp(ext, _T(".epub")) && _tcscmp(ext, _T(".ol")))
    {
        MessageBox_(hWnd, IDS_UNKNOWN_FORMAT, IDS_ERROR, MB_OK | MB_ICONERROR);
        return;
    }

#if ENABLE_MD5
    if (!Book::CalcMd5(szFileName, &md5, &data, &size))
    {
        MessageBox_(hWnd, IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_OK | MB_ICONERROR);
        return;
    }
#else
    fp = _tfopen(szFileName, _T("rb"));
    if (!fp)
    {
        MessageBox_(hWnd, IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_OK | MB_ICONERROR);
        return;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
#endif

    if (size == 0)
    {
#if ENABLE_MD5
        free(data);
#endif
        MessageBox_(hWnd, IDS_EMPTY_FILE, IDS_ERROR, MB_OK | MB_ICONERROR);
        return;
    }

    // check md5, is already exist
#if ENABLE_MD5
    item = _Cache.find_item(&md5, szFileName);
#else
    item = _Cache.find_item(szFileName);
#endif
    if (item)
    {
        if (!forced)
        {
            if (_item && item->id == _item->id && !_Book->IsLoading()) // current is opened
            {
#if ENABLE_MD5
                free(data);
#endif
                OnUpdateMenu(hWnd);
                return;
            }
        }
    }

    if (_Book)
    {
        delete _Book;
        _Book = NULL;
    }

    if (_tcscmp(ext, _T(".txt")) == 0)
    {
#if ENABLE_MD5
        free(data);
        data = NULL;
#endif
        _Book = new TextBook;
#if ENABLE_MD5
        _Book->SetMd5(&md5);
#endif
        _Book->SetFileName(szFileName);
        _Book->SetChapterRule(&(_header->chapter_rule));
        _Book->OpenBook(NULL, size, hWnd);
    }
    else if (_tcscmp(ext, _T(".epub")) == 0)
    {
#if ENABLE_MD5
        free(data);
        data = NULL;
#endif
        _Book = new EpubBook;
#if ENABLE_MD5
        _Book->SetMd5(&md5);
#endif
        _Book->SetFileName(szFileName);
        _Book->OpenBook(hWnd);
}
    else if (_tcscmp(ext, _T(".ol")) == 0)
    {
#if ENABLE_NETWORK
        arg = GetCheckBookArguments();
        if (arg && arg->book && arg->book->GetBookType() == book_online
            && 0 == _tcscmp(arg->book->GetFileName(), szFileName))
        {
            delete arg->book;
            arg->book = NULL;
        }
#endif
        _Book = new OnlineBook;
        _Book->SetFileName(szFileName);
        _Book->OpenBook(NULL, size, hWnd);
    }
    
    //EnableWindow(hWnd, FALSE);
    PlayLoadingImage(hWnd);
}

void OnOpenOlBook(HWND hWnd, void* olparam)
{
    static TCHAR oldir[MAX_PATH] = { 0 };
    TCHAR savepath[MAX_PATH] = { 0 };
    ol_book_param_t* param = (ol_book_param_t*)olparam;
    ol_header_t olheader = { 0 };
    FILE* fp = NULL;
    int i;

    // create save path
    if (!oldir[0])
    {
        // generate online file save path
        GetModuleFileName(NULL, oldir, sizeof(TCHAR) * (MAX_PATH - 1));
        for (i = _tcslen(oldir) - 1; i >= 0; i--)
        {
            if (oldir[i] == _T('\\') || oldir[i] == _T('/'))
            {
                memcpy(&oldir[i + 1], ONLINE_FILE_SAVE_PATH, (_tcslen(ONLINE_FILE_SAVE_PATH) + 1) * sizeof(TCHAR));
                break;
            }
        }

        // create path
        if (CreateDirectory(oldir, NULL))
        {
            SetFileAttributes(oldir, FILE_ATTRIBUTE_HIDDEN);
        }
    }
    _tcscpy(savepath, oldir);
    _tcscat(savepath, param->book_name);
    _tcscat(savepath, _T(".ol"));

    // check .ol file is exist
    if (PathFileExists(savepath))
    {
        OnOpenBook(hWnd, savepath, FALSE);
        return;
    }

    // generate ol_header_t
    olheader.header_size = sizeof(ol_header_t) - sizeof(ol_chapter_info_t);
    olheader.book_name_offset = olheader.header_size;
    olheader.header_size += (_tcslen(param->book_name) + 1) * sizeof(TCHAR);
    olheader.main_page_offset = olheader.header_size;
    olheader.header_size += (strlen(param->main_page) + 1) * sizeof(char);
    olheader.host_offset = olheader.header_size;
    olheader.header_size += (strlen(param->host) + 1) * sizeof(char);
    olheader.update_time = 0;
    olheader.is_finished = param->is_finished;
    olheader.chapter_size = 0;

    // write file
    fp = _tfopen(savepath, _T("wb"));
    fwrite(&olheader, 1, olheader.book_name_offset, fp);
    fwrite(param->book_name, 1, olheader.main_page_offset - olheader.book_name_offset, fp);
    fwrite(param->main_page, 1, olheader.host_offset - olheader.main_page_offset, fp);
    fwrite(param->host, 1, olheader.header_size - olheader.host_offset, fp);
    fclose(fp);

    OnOpenBook(hWnd, savepath, FALSE);
}

VOID GetCacheVersion(TCHAR *ver)
{
    TCHAR version[256] = { 0 };
    Utils::GetApplicationVersion(version);
    wcscpy(ver, version);
}

BOOL Init(void)
{
    if (!_Cache.init())
    {
        MessageBox_(NULL, IDS_INIT_CACHE_FAIL, IDS_ERROR, MB_OK);
        return FALSE;
    }

    _header = _Cache.get_header();

    // delete not exist items
    for (int i=0; i<_header->item_count; i++)
    {
        item_t* item = _Cache.get_item(i);
        if (!PathFileExists(item->file_name))
        {
            _Cache.delete_item(i);
            i--;
        }
    }

#if ENABLE_NETWORK
    // set proxy for http client
    HttpClient::Instance()->SetProxy(&_header->proxy);
#endif

    return TRUE;
}

void Exit(void)
{
    if (_Book)
    {
        delete _Book;
        _Book = NULL;
    }

    if (!_Cache.exit())
    {
        MessageBox_(NULL, IDS_SAVE_CACHE_FAIL, IDS_ERROR, MB_OK);
    }
#if ENABLE_NETWORK
    HtmlParser::ReleaseInstance();
    HttpClient::ReleaseInstance();
#endif
}

void Save(HWND hWnd)
{
#if ENABLE_REALTIME_SAVE
    PostMessage(hWnd, WM_SAVE_CACHE, 0, NULL);
#endif
}

LRESULT OnSave(HWND hWnd)
{
#if ENABLE_REALTIME_SAVE
    RECT rc, rcbak;
    LOGFONT fontbak;
#if ENABLE_TAG
    LOGFONT tagfontbak[MAX_TAG_COUNT] = { 0 };
#endif

    // save rect
    rcbak = _header->rect;
    fontbak = _header->font;
    if (_WndInfo.bHideBorder && !_WndInfo.bFullScreen)
    {
        GetClientRectExceptStatusBar(hWnd, &rc);
        ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.left));
        ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.right));
        rc.left = rc.left - _WndInfo.hbRect.left;
        rc.right = rc.right + _WndInfo.hbRect.right;
        rc.top = rc.top - _WndInfo.hbRect.top;
        rc.bottom = rc.bottom + _WndInfo.hbRect.bottom;
        _header->rect = rc;
    }
    RestoreRectForDpi(&_header->rect);
    RestoreFontForDpi(&_header->font);
#if ENABLE_TAG
    for (int i = 0; i < MAX_TAG_COUNT; i++)
    {
        tagfontbak[i] = _header->tags[i].font;
        RestoreFontForDpi(&_header->tags[i].font);
    }
#endif

    // save
    _Cache.save();

    // restore
    _header->rect = rcbak;
    _header->font = fontbak;
#if ENABLE_TAG
    for (int i = 0; i < MAX_TAG_COUNT; i++)
    {
        _header->tags[i].font = tagfontbak[i];
    }
#endif
#endif
    return 0;
}

void UpdateProgess(void)
{
    TCHAR progress[256] = {0};
    TCHAR str[256] = { 0 };
    double dprog = 0.0;
    int nprog = 0;

    if (EC_IsEditMode())
    {
        LoadString(hInst, IDS_EDIT_MODE_TIPS, progress, 256);
        SendMessage(_WndInfo.hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)progress);
        return;
    }

    if (_item && _Book && !_Book->IsLoading())
    {
        dprog = (double)_Book->GetProgress();
        nprog = (int)(dprog * 100);
        dprog = (double)nprog / 100.0;
        if (!_IsAutoPage)
        {
            _stprintf(progress, _T("  %.2f%%  ( %d / %d )"), dprog, _item->index + _Book->GetCurPageSize(), _Book->GetTextLength());
        }
        else
        {
            LoadString(hInst, IDS_AUTOPAGING, str, 256);
            _stprintf(progress, _T("  %.2f%%  ( %d / %d )  [%s]"), dprog, _item->index + _Book->GetCurPageSize(), _Book->GetTextLength(), str);
        }
        SendMessage(_WndInfo.hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)progress);
    }
    else
    {
        SendMessage(_WndInfo.hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
    }
}

void UpdateTitle(HWND hWnd)
{
    TCHAR szOldTitle[MAX_LOADSTRING + MAX_PATH + MAX_PATH] = { 0 };
    TCHAR szNewTitle[MAX_LOADSTRING + MAX_PATH + MAX_PATH] = { 0 };
    TCHAR szFileName[MAX_PATH] = { 0 };
    TCHAR szChapter[MAX_PATH] = { 0 };

    if (!_item || !_Book || _Book->IsLoading())
        return;

    memcpy(szFileName, _item->file_name, sizeof(szFileName));
    PathRemoveExtension(szFileName);

    if (_Book && !_Book->IsLoading())
    {
        if (_Book->GetChapterTitle(szChapter, MAX_PATH))
        {
            _stprintf(szNewTitle, _T("%s - %s - %s"), szTitle, PathFindFileName(szFileName), szChapter);
        }
    }
    
    if (!szNewTitle[0])
    {
        _stprintf(szNewTitle, _T("%s - %s"), szTitle, PathFindFileName(szFileName));
    }

    GetWindowText(hWnd, szOldTitle, MAX_LOADSTRING + MAX_PATH + MAX_PATH - 1);
    if (_tcscmp(szOldTitle, szNewTitle))
        SetWindowText(hWnd, szNewTitle);
}

void RemoveMenuById(HMENU hMenu, BOOL separator, int id)
{
    HMENU hSubMenu;
    int count;
    int i,j;
    int menuid;

    if (!hMenu)
        return;

    count = GetMenuItemCount(hMenu);
    for (i=0; i<count; i++)
    {
        hSubMenu = GetSubMenu(hMenu, i);
        if (hSubMenu)
            RemoveMenuById(hSubMenu, separator, id);
        menuid = GetMenuItemID(hMenu, i);
        if (id == menuid)
        {
            if (separator)
            {
                j = i+1;
                RemoveMenu(hMenu, j, MF_BYPOSITION);
            }
            RemoveMenu(hMenu, id, MF_BYCOMMAND);
            return;
        }
    }
}

void RemoveMenus(HWND hWnd, BOOL redraw)
{
    HMENU hMenu;

    hMenu = GetMenu(hWnd);

#if !ENABLE_NETWORK
    RemoveMenuById(hMenu, TRUE, IDM_PROXY);
    RemoveMenuById(hMenu, TRUE, IDM_ONLINE);
#endif

#if !ENABLE_TAG
    RemoveMenuById(hMenu, FALSE, IDM_TAGSET);
#endif

    if (redraw)
        DrawMenuBar(hWnd);
}

BOOL GetClientRectExceptStatusBar(HWND hWnd, RECT* rc)
{
    RECT rect;
    GetClientRect(_WndInfo.hStatusBar, &rect);
    GetClientRect(hWnd, rc);
    if (IsWindowVisible(_WndInfo.hStatusBar))
    {
        rc->bottom -= rect.bottom;
    }
    return TRUE;
}

void WheelSpeedInit(HWND hDlg)
{
    int i;
    HWND hWnd = NULL;
    TCHAR buf[8] = {0};

    hWnd = GetDlgItem(hDlg, IDC_COMBO_SPEED);
    if (_Book)
    {
        for (i=1; i<=_Book->GetOnePageLineCount(); i++)
        {
            _itot(i, buf, 10);
            SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)buf);
        }
    }
    else
    {
        for (i=1; i<=_header->wheel_speed; i++)
        {
            _itot(i, buf, 10);
            SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)buf);
        }
    }
    SendMessage(hWnd, CB_SETCURSEL, _header->wheel_speed - 1, NULL);
}

Bitmap* LoadBGImage(int w, int h, BYTE alpha)
{
    static Bitmap *bgimg = NULL;
    static TCHAR curfile[MAX_PATH] = {0};
    static int curWidth = 0;
    static int curHeight = 0;
    static int curmode = Stretch;
    static BYTE s_alpha = 0xFF;
    Bitmap *image = NULL;
    Graphics *graphics = NULL;
    ImageAttributes ImgAtt;
    RectF rcDrawRect;

    if (!_header->bg_image.enable || !_header->bg_image.file_name[0])
    {
        return NULL;
    }

    if (_tcscmp(_header->bg_image.file_name, curfile) == 0 && curWidth == w && curHeight == h 
        && curmode == _header->bg_image.mode && bgimg && alpha == s_alpha)
    {
        return bgimg;
    }

    if (!FileExists(_header->bg_image.file_name))
    {
        _header->bg_image.file_name[0] = 0;
        return NULL;
    }
    
    if (bgimg)
    {
        delete bgimg;
        bgimg = NULL;
    }
    _tcscpy(curfile, _header->bg_image.file_name);
    curWidth = w;
    curHeight = h;
    curmode = _header->bg_image.mode;
    s_alpha = alpha;
    
    // load image file
    image = Bitmap::FromFile(curfile);
    if (image == NULL)
        return NULL;
    if (Gdiplus::Ok != image->GetLastStatus())
    {
        delete image;
        image = NULL;
        return NULL;
    }

    // create bg image
    bgimg = new Bitmap(curWidth, curHeight, PixelFormat32bppARGB);
    rcDrawRect.X=0.0;
    rcDrawRect.Y=0.0;
    rcDrawRect.Width=(float)curWidth;
    rcDrawRect.Height=(float)curHeight;
    graphics = Graphics::FromImage(bgimg);
    graphics->SetInterpolationMode(InterpolationModeHighQualityBicubic);

    ColorMatrix colorMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, alpha/255.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f
    };
    ImgAtt.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
    switch (curmode)
    {
    case Stretch:
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)image->GetWidth(),(float)image->GetHeight(),UnitPixel,&ImgAtt);
        break;
    case Tile:
        ImgAtt.SetWrapMode(WrapModeTile);
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)curWidth,(float)curHeight,UnitPixel,&ImgAtt);
        break;
    case TileFlip:
        ImgAtt.SetWrapMode(WrapModeTileFlipXY);
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)curWidth,(float)curHeight,UnitPixel,&ImgAtt);
        break;
    default:
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)image->GetWidth(),(float)image->GetHeight(),UnitPixel,&ImgAtt);
        break;
    }

    delete image;
    delete graphics;
    return bgimg;
}

BOOL FileExists(TCHAR *file)
{
    DWORD dwAttrib = GetFileAttributes(file);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void StartAutoPage(HWND hWnd)
{
    if (_item && !_IsAutoPage && _Book && !_Book->IsLoading())
    {
        if ((_header->autopage_mode & 0xf0) == apm_count)
        {
            SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse * _Book->GetCurPageSize(), NULL);
        }
        else
        {
            SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse, NULL);
        }
        _IsAutoPage = TRUE;
        UpdateProgess();
    }
}

void StopAutoPage(HWND hWnd)
{
    if (_IsAutoPage)
    {
        KillTimer(hWnd, IDT_TIMER_PAGE);
        _IsAutoPage = FALSE;
        UpdateProgess();
    }
}

void PauseAutoPage(HWND hWnd)
{
    if (_IsAutoPage)
    {
        KillTimer(hWnd, IDT_TIMER_PAGE);
    }
}

void ResumeAutoPage(HWND hWnd)
{
    if (_item && _IsAutoPage && _Book && !_Book->IsLoading())
    {
        if ((_header->autopage_mode & 0xf0) == apm_count)
        {
            SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse * _Book->GetCurPageSize(), NULL);
        }
        else
        {
            SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse, NULL);
        }
    }
    else
    {
        KillTimer(hWnd, IDT_TIMER_PAGE);
        _IsAutoPage = FALSE;
    }
}

void ResetAutoPage(HWND hWnd)
{
    if (_item && _IsAutoPage && _Book && !_Book->IsLoading())
    {
        if ((_header->autopage_mode & 0xf0) == apm_count)
        {
            KillTimer(hWnd, IDT_TIMER_PAGE);
            SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse * _Book->GetCurPageSize(), NULL);
        }
    }
}

#if ENABLE_NETWORK
void CheckUpgrade(HWND hWnd)
{
    // set proxy & check upgrade
    _Upgrade.SetProxy(&_header->proxy);
    _Upgrade.SetIngoreVersion(_header->ingore_version);
    _Upgrade.Check(UpgradeCallback, hWnd);
    SetTimer(hWnd, IDT_TIMER_UPGRADE, 24 * 60 * 60 * 1000 /*one day*/, NULL);
}

bool UpgradeCallback(void *param, json_item_data_t *item)
{
    HWND hWnd = (HWND)param;

    PostMessage(hWnd, WM_NEW_VERSION, 0, NULL);
    return true;
}

chkbook_arg_t* GetCheckBookArguments()
{
    static chkbook_arg_t* s_arg = NULL;
    if (!s_arg)
    {
        s_arg = new chkbook_arg_t;
    }
    return s_arg;
}

void StartCheckBookUpdate(HWND hWnd)
{
    KillTimer(hWnd, IDT_TIMER_CHECKBOOK);
    SetTimer(hWnd, IDT_TIMER_CHECKBOOK, 60 * 1000 /*one minute*/, NULL);
}

void OnCheckBookUpdateCallback(int is_update, int err, void* param)
{
    chkbook_arg_t* arg = (chkbook_arg_t*)param;
    int i;
#if TEST_MODEL
    char msg[1024] = { 0 };
    char* ansi = NULL;
    int len;
#endif

    if (!arg)
        return;

    if (!arg->book)
        return;

#if TEST_MODEL
    ansi = Utils::utf16_to_ansi(arg->book->GetFileName(), &len);
    sprintf(msg, "{%s:%d} file=%s\n", __FUNCTION__, __LINE__, ansi);
    OutputDebugStringA(msg);
    free(ansi);
#endif

    if (is_update)
    {
        for (i = 0; i < _header->item_count; i++)
        {
            item_t* item = _Cache.get_item(i);
            if (_tcscmp(item->file_name, arg->book->GetFileName()) == 0)
            {
                item->is_new = TRUE;
                Save(arg->hWnd);
                break;
            }
        }

        OnUpdateMenu(arg->hWnd);
    }

    if (arg->book)
    {
        delete arg->book;
        arg->book = NULL;
    }   

    // check next book
    if (arg->hWnd)
    {
        KillTimer(arg->hWnd, IDT_TIMER_CHECKBOOK);
        SetTimer(arg->hWnd, IDT_TIMER_CHECKBOOK, 60 * 1000 /*one minute*/, NULL);
    }
}

void OnCheckBookUpdate(HWND hWnd)
{
    chkbook_arg_t* arg = GetCheckBookArguments();
    item_t* item = NULL;
    int i;
    int found = -1;

    if (!arg)
        return;

    if (arg->book)
    {
        delete arg->book;
        arg->book = NULL;
    }   

_check_next:
    found = -1;
    // find an online book
    for (i = 0; i < _header->item_count; i++)
    {
        item = _Cache.get_item(i);
        if (0 == _tcscmp(PathFindExtension(item->file_name), _T(".ol")))
        {
            if (arg->checked_list.find(item->file_name) != arg->checked_list.end())
            {
                continue;
            }
            found = i;
            break;
        }
    }

    if (found != -1)
    {
        item = _Cache.get_item(found);
        if (_Book && _Book->GetBookType() == book_online && _tcscmp(_Book->GetFileName(), item->file_name) == 0)
        {
            arg->hWnd = hWnd;
            arg->checked_list.insert(item->file_name);
            if (2 != ((OnlineBook*)_Book)->CheckUpdate(hWnd, OnCheckBookUpdateCallback, arg))
            {
                found = -1;
                goto _check_next;
            }
        }
        else
        {
            arg->hWnd = hWnd;
            arg->checked_list.insert(item->file_name);
            arg->book = new OnlineBook;
            arg->book->SetFileName(item->file_name);
            if (2 != arg->book->CheckUpdate(hWnd, OnCheckBookUpdateCallback, arg))
            {
                delete arg->book;
                arg->book = NULL;

                found = -1;
                goto _check_next;
            }
        }
    }
    else
    {
        if (arg->book)
        {
            delete arg->book;
            arg->book = NULL;
        }
        arg->checked_list.clear();
        KillTimer(hWnd, IDT_TIMER_CHECKBOOK);
        SetTimer(hWnd, IDT_TIMER_CHECKBOOK, 60 * 60 * 1000 /*one hour*/, NULL);
    }
}
#endif

bool PlayLoadingImage(HWND hWnd)
{
    UINT count;
    GUID *pDimensionIDs = NULL;
    bool ret = false;
    WCHAR strGuid[39];
    UINT size;
    GUID Guid = FrameDimensionTime;
    RECT rect;

    if (!_loading)
    {
        // create loading data
        _loading = (loading_data_t *)malloc(sizeof(loading_data_t));
        memset(_loading, 0, sizeof(loading_data_t));

        // load image from resource
        if (!LoadResourceImage(MAKEINTRESOURCE(IDR_GIF_LOADING), _T("GIF"), &(_loading->image), &(_loading->hMemory)))
        {
            free(_loading);
            _loading = NULL;
            return false;
        }

        count = _loading->image->GetFrameDimensionsCount();
        pDimensionIDs = (GUID*)malloc(sizeof(GUID)*count);
        _loading->image->GetFrameDimensionsList(pDimensionIDs, count);
        StringFromGUID2(pDimensionIDs[0], strGuid, 39);
        _loading->frameCount = _loading->image->GetFrameCount(&pDimensionIDs[0]);
        free(pDimensionIDs);
        size = _loading->image->GetPropertyItemSize(PropertyTagFrameDelay);
        _loading->item = (PropertyItem*)malloc(size);
        _loading->image->GetPropertyItem(PropertyTagFrameDelay, size, _loading->item);
    }

    _loading->enable = TRUE;
    _loading->frameIndex = 0;
    Guid = FrameDimensionTime;
    _loading->image->SelectActiveFrame(&Guid, _loading->frameIndex);

    SetTimer(hWnd, IDT_TIMER_LOADING, ((UINT*)_loading->item->value)[_loading->frameIndex] * 10, NULL);
    _loading->frameIndex = (++ _loading->frameIndex) % _loading->frameCount;
    GetClientRectExceptStatusBar(hWnd, &rect);
    InvalidateRect(hWnd, &rect, FALSE);
    return true;
}

bool StopLoadingImage(HWND hWnd)
{
    RECT rect;

    if (!_loading)
        return false;
    _loading->enable = FALSE;
    KillTimer(hWnd, IDT_TIMER_LOADING);
    GetClientRectExceptStatusBar(hWnd, &rect);
    InvalidateRect(hWnd, &rect, FALSE);
    return true;
}

ULONGLONG GetDllVersion(LPCTSTR lpszDllName)
{
    ULONGLONG ullVersion = 0;
    HINSTANCE hinstDll;
    hinstDll = LoadLibrary(lpszDllName);
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");
        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;
            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);
            hr = (*pDllGetVersion)(&dvi);
            if(SUCCEEDED(hr))
                ullVersion = MAKEDLLVERULL(dvi.dwMajorVersion, dvi.dwMinorVersion,0,0);
        }
        FreeLibrary(hinstDll);
    }
    return ullVersion;
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    DWORD ProcessId;
    TCHAR szProcessName[MAX_PATH] = _T("<unknown>");
    int *flag = (int*)lParam;
    
    GetWindowThreadProcessId(hWnd, &ProcessId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
    if (hProcess)
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
        {
            GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(TCHAR));
            if (_tcscmp(_T("Reader.exe"), szProcessName) == 0)
            {
                ShowWindow(hWnd, SW_SHOW);
                SetForegroundWindow(hWnd);
                (*flag) += 1;
                return (*flag) > 1 ? FALSE : TRUE;
            }
        }
    }
    return TRUE;
}

void ShowInTaskbar(HWND hWnd, BOOL bShow)
{
    HRESULT hr;
    ITaskbarList* pTaskbarList;
    hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList, (void**)&pTaskbarList );

    if(SUCCEEDED(hr))
    {
        pTaskbarList->HrInit();
        if(bShow)
            pTaskbarList->AddTab(hWnd);
        else
            pTaskbarList->DeleteTab(hWnd);

        pTaskbarList->Release();
    }
}

void ShowSysTray(HWND hWnd, BOOL bShow)
{
    if (bShow)
    {
        if (!_nid.hWnd)
        {
            // create a systray
            ULONGLONG ullVersion = GetDllVersion(_T("Shell32.dll"));
            ZeroMemory(&_nid, sizeof(NOTIFYICONDATA));
            if(ullVersion >= MAKEDLLVERULL(6,0,0,0))
                _nid.cbSize = sizeof(NOTIFYICONDATA);
            else if(ullVersion >= MAKEDLLVERULL(5,0,0,0))
                _nid.cbSize = NOTIFYICONDATA_V2_SIZE;
            else
                _nid.cbSize = NOTIFYICONDATA_V1_SIZE;
            _nid.hWnd = hWnd;
            _nid.uID = IDI_SYSTRAY;
            _nid.uCallbackMessage = WM_SYSTRAY;
            _nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BOOK));
            _tcscpy(_nid.szTip, szTitle);
        }
        if (_nid.uFlags == 0)
        {
            _nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_INFO;
            Shell_NotifyIcon(NIM_ADD, &_nid);
        }
    }
    else
    {
        if (_nid.uFlags)
        {
            _nid.uFlags = 0;
            Shell_NotifyIcon(NIM_DELETE, &_nid);
        }
    }
}

HBITMAP CreateAlphaTextBitmap(HWND hWnd, HFONT inFont, COLORREF inColour, int width, int height)
{
    // Create DC and select font into it
    HDC hTextDC = CreateCompatibleDC(NULL);
    HFONT hOldFont = (HFONT)SelectObject(hTextDC, inFont);
    HBITMAP hDIB = NULL;
    BITMAPINFOHEADER BMIH;
    void *pvBits = NULL;
    HBITMAP hOldBMP = NULL;
    BYTE* DataPtr = NULL;
    BYTE FillR,FillG,FillB,ThisA;
    int x,y;

    // Specify DIB setup
    memset(&BMIH, 0x0, sizeof(BITMAPINFOHEADER));
    BMIH.biSize = sizeof(BMIH);
    BMIH.biWidth = width;
    BMIH.biHeight = height;
    BMIH.biPlanes = 1;
    BMIH.biBitCount = 32;
    BMIH.biCompression = BI_RGB;

    // Create and select DIB into DC
    hDIB = CreateDIBSection(hTextDC, (LPBITMAPINFO)&BMIH, 0, (LPVOID*)&pvBits, NULL, 0); 
    hOldBMP = (HBITMAP)SelectObject(hTextDC, hDIB);
    if (hOldBMP)
    {
        // Set up DC properties 
        SetTextColor(hTextDC, 0x00FFFFFF);
        SetBkColor(hTextDC, 0x00000000);
        SetBkMode(hTextDC, OPAQUE);

        // Draw text to buffer
        _Book->DrawPage(hWnd, hTextDC);
        if (!_Book->IsCoverPage())
        {
            DataPtr = (BYTE*)pvBits;
            FillR = GetRValue(inColour);
            FillG = GetGValue(inColour);
            FillB = GetBValue(inColour);
            for (y = 0; y < BMIH.biHeight; y++)
            { 
                for (x = 0; x < BMIH.biWidth; x++)
                { 
                    ThisA = *DataPtr; // Move alpha and pre-multiply with RGB 
                    *DataPtr++ = (FillB * ThisA) >> 8; 
                    *DataPtr++ = (FillG * ThisA) >> 8; 
                    *DataPtr++ = (FillR * ThisA) >> 8;
#if 0
                    *DataPtr++ = ThisA; // Set Alpha 
#else
                    *DataPtr++ = ThisA > _textAlpha ? _textAlpha : ThisA;
#endif
                }
            }
        }

        // De-select bitmap
        SelectObject(hTextDC, hOldBMP);
    }

    // De-select font and destroy temp DC
    SelectObject(hTextDC, hOldFont);
    DeleteDC(hTextDC);

    // Return DIBSection 
    return hDIB;
}

void SetTreeviewFont()
{
    static HFONT s_hFont = NULL;

    if (s_hFont)
    {
        DeleteObject(s_hFont);
        s_hFont = NULL;
    }

    if (_header->meun_font_follow)
    {
        s_hFont = CreateFontIndirect(&_header->font);
        int height = abs((int)(_header->font.lfHeight * 1.5f));
        SendMessage(_hTreeView, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeView, TVM_SETITEMHEIGHT, height, NULL);
        SendMessage(_hTreeMark, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeMark, TVM_SETITEMHEIGHT, height, NULL);
    }
    else
    {
        NONCLIENTMETRICS theMetrics = {0};
        theMetrics.cbSize = sizeof(NONCLIENTMETRICS);
        if (Utils::isWindowsXP())
            theMetrics.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(theMetrics.iPaddedBorderWidth);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS), (PVOID) &theMetrics,0);
        theMetrics.iMenuHeight = abs((int)(theMetrics.iMenuHeight * 1.25f)); // 1.25 line height
        if (theMetrics.iMenuHeight == 0)
        {
            theMetrics.iMenuHeight = 20;
        }
        s_hFont = CreateFontIndirect(&(theMetrics.lfMenuFont));
        SendMessage(_hTreeView, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeView, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
        SendMessage(_hTreeMark, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeMark, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
    }
}

BOOL LoadResourceImage(LPCWSTR lpName, LPCWSTR lpType, Bitmap **image, HGLOBAL *hMemory)
{
    HRSRC hResource;
    HGLOBAL hResLoad;
    const void* pResourceData = NULL;
    IStream* pStream = NULL;
    DWORD imageSize;
    void* buffer = NULL;

    hResource = ::FindResource(hInst, lpName, lpType);
    if (!hResource)
        return FALSE;

    hResLoad = ::LoadResource(hInst, hResource);
    if (!hResLoad)
        return FALSE;

    pResourceData = ::LockResource(hResLoad);
    if (!pResourceData)
    {
        ::FreeResource(hResLoad);
        return FALSE;
    }

    imageSize = ::SizeofResource(hInst, hResource);
    if (0 == imageSize)
    {
        ::FreeResource(hResLoad);
        return FALSE;
    }

    *hMemory = ::GlobalAlloc(GMEM_MOVEABLE, imageSize);
    if (!(*hMemory))
    {
        ::FreeResource(hResLoad);
        return FALSE;
    }

    buffer = ::GlobalLock(*hMemory);
    if (!buffer)
    {
        ::FreeResource(hResLoad);
        ::GlobalFree(*hMemory);
        return FALSE;
    }

    CopyMemory(buffer, pResourceData, imageSize);

    if (::CreateStreamOnHGlobal(*hMemory, FALSE, &pStream) == S_OK)
    {
        *image = Gdiplus::Bitmap::FromStream(pStream);
        pStream->Release();
    }
    ::FreeResource(hResLoad);
    ::GlobalUnlock(*hMemory);
    //::GlobalFree(hMemory);

    if ((*image) && Gdiplus::Ok == (*image)->GetLastStatus())
    {
        return TRUE;
    }

    if (*image)
        delete (*image);
    *image = NULL;
    ::GlobalFree(*hMemory);
    *hMemory = NULL;
    return FALSE;
}

book_source_t* FindBookSource(const char* host)
{
    int i = 0;

    if (!host)
        return NULL;
    for (i = 0; i < _header->book_source_count; i++)
    {
        if (0 == strcmp(_header->book_sources[i].host, host))
            return &_header->book_sources[i];
    }
    return NULL;
}

HHOOK hMsgBoxHook = NULL;
LRESULT CALLBACK MsgBoxHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    HWND hParent;
    RECT rc;
    int w, h, x, y;
    LPCBT_CREATEWND cbt;
    if (nCode == HCBT_CREATEWND)
    {
        cbt = ((LPCBT_CREATEWND)lParam);

        if (cbt->lpcs->lpszClass == (LPWSTR)(ATOM)32770)  // #32770 = dialog box class
        {
            hParent = cbt->lpcs->hwndParent;
            if (hParent)
            {
                GetWindowRect(hParent, &rc);
                w = rc.right - rc.left;
                h = rc.bottom - rc.top;
                x = rc.left + (w - cbt->lpcs->cx) / 2;
                y = rc.top + (h - cbt->lpcs->cy) / 2;
                cbt->lpcs->x = x;
                cbt->lpcs->y = y;
            }
        }
    }

    return CallNextHookEx(hMsgBoxHook, nCode, wParam, lParam);
}

int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType)
{
    TCHAR szText[MAX_LOADSTRING] = { 0 };
    TCHAR szCaption[MAX_LOADSTRING] = { 0 };
    int res;

    hMsgBoxHook = SetWindowsHookEx(WH_CBT, MsgBoxHookProc, hInst, GetCurrentThreadId());
    LoadString(hInst, textId, szText, MAX_LOADSTRING);
    LoadString(hInst, captionId, szCaption, MAX_LOADSTRING);
    res = MessageBoxEx(hWnd, szText, szCaption, uType, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

    if (hMsgBoxHook)
        UnhookWindowsHookEx(hMsgBoxHook);
    return res;
}

int MessageBoxFmt_(HWND hWnd, UINT captionId, UINT uType, UINT formatId, ...)
{
    TCHAR szText[MAX_LOADSTRING] = { 0 };
    TCHAR szCaption[MAX_LOADSTRING] = { 0 };
    TCHAR szFormat[MAX_LOADSTRING] = { 0 };
    int res;
    va_list args;

    hMsgBoxHook = SetWindowsHookEx(WH_CBT, MsgBoxHookProc, hInst, GetCurrentThreadId());
    LoadString(hInst, formatId, szFormat, MAX_LOADSTRING);
    LoadString(hInst, captionId, szCaption, MAX_LOADSTRING);

    va_start(args, formatId);
    _sntprintf(szText, MAX_LOADSTRING, szFormat, args);
    va_end(args);
    
    res = MessageBoxEx(hWnd, szText, szCaption, uType, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

    if (hMsgBoxHook)
        UnhookWindowsHookEx(hMsgBoxHook);
    return res;
}
