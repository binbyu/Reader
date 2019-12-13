// Reader.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Reader.h"
#include <stdio.h>
#include <shlwapi.h>
#include <CommDlg.h>
#include <commctrl.h>
#include <vector>
#include <shellapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "shell32.lib")


#define MAX_LOADSTRING              100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szStatusClass[MAX_LOADSTRING];			// the status window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Setting(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    JumpProgress(HWND, UINT, WPARAM, LPARAM);



int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

    // load cache file
    if (!Init())
    {
        return FALSE;
    }

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_READER, szWindowClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_STATUSBAR, szStatusClass, MAX_LOADSTRING);
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

	wcex.style			= CS_HREDRAW | CS_VREDRAW /*| CS_DBLCLKS*/;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BOOK));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= CreateSolidBrush(_header->bk_color);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_READER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_BOOK));

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

   hWnd = CreateWindowEx(WS_EX_ACCEPTFILES, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      _header->rect.left, _header->rect.top, 
      _header->rect.right - _header->rect.left,
      _header->rect.bottom - _header->rect.top,
      NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
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
    static BOOL hiden = FALSE;
    static RECT border;
    static DWORD dwStyle;
    static HMENU hMenu;

    if (message == _uFindReplaceMsg)
    {
        OnFindText(hWnd, message, wParam, lParam);
        return 0;
    }

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
        if (wmId >= IDM_CHAPTER_BEGIN && wmId <= IDM_CHAPTER_END)
        {
            if (_ChapterMap.end() != _ChapterMap.find(wmId))
            {
                _item->index = _ChapterMap[wmId];
                _PageCache.Reset(hWnd);
            }
            break;
        }
        else if (wmId >= IDM_OPEN_BEGIN && wmId <= IDM_OPEN_END)
        {
            int item_id = wmId - IDM_OPEN_BEGIN;
            OnOpenItem(hWnd, item_id);
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
        case IDM_FONT:
            OnSetFont(hWnd, message, wParam, lParam);
            break;
        case IDM_COLOR:
            OnSetBkColor(hWnd, message, wParam, lParam);
            break;
        case IDM_CONFIG:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTING), hWnd, Setting);
            break;
        case IDM_DEFAULT:
            OnRestoreDefault(hWnd, message, wParam, lParam);
            break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
    case WM_CREATE:
        OnCreate(hWnd);
        // register hot key
        RegisterHotKey(hWnd, ID_HOTKEY_SHOW_HIDE_WINDOW, _header->hk_show_1 | _header->hk_show_2 | MOD_NOREPEAT, _header->hk_show_3);
        break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
        OnPaint(hWnd, hdc);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
        UnregisterHotKey(hWnd, ID_HOTKEY_SHOW_HIDE_WINDOW);
        UnregisterHotKey(hWnd, ID_HOTKEY_TOP_WINDOW);
        GetWindowRect(hWnd, &rc);
        if (hiden)
        {
            rc.left -= border.left;
            rc.top -= border.top;
            rc.right += border.right;
            rc.bottom += border.bottom;
        }
        _header->rect.left = rc.left;
        _header->rect.right = rc.right;
        _header->rect.top = rc.top;
        _header->rect.bottom = rc.bottom;
		PostQuitMessage(0);
		break;
    case WM_NCHITTEST:
        hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit == HTCLIENT && hiden)
        {
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            GetClientRectExceptStatusBar(hWnd, &rc);
            rc.bottom = rc.bottom/2 > 80 ? 80 : rc.bottom/2;
            if (PtInRect(&rc, pt))
                hit = HTCAPTION;
        }
        return hit;
    case WM_SIZE:
        OnSize(hWnd, message, wParam, lParam);
        break;
    case WM_MOVE:
        OnMove(hWnd);
        break;
    case WM_KEYDOWN:
        if (VK_LEFT == wParam)
        {
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                OnGotoPrevChapter(hWnd, message, wParam, lParam);
            }
            else
            {
                OnPageUp(hWnd);
            }
        }
        else if (VK_RIGHT == wParam)
        {
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                OnGotoNextChapter(hWnd, message, wParam, lParam);
            }
            else
            {
                OnPageDown(hWnd);
            }
        }
        else if (VK_UP == wParam)
        {
            OnLineUp(hWnd);
        }
        else if (VK_DOWN == wParam)
        {
            OnLineDown(hWnd);
        }
        else if ('F' == wParam)
        {
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                OnFindText(hWnd, message, wParam, lParam);
            }
        }
        else if ('T' == wParam)
        {
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                // topmost window
                static bool isTopmost = false;
                SetWindowPos(hWnd, isTopmost ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
                isTopmost = !isTopmost;
            }
        }
        else if ('O' == wParam)
        {
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                OnOpenFile(hWnd, message, wParam, lParam);
            }
        }
        else if ('G' == wParam)
        {
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                DialogBox(hInst, MAKEINTRESOURCE(IDD_JUMP_PROGRESS), hWnd, JumpProgress);
                break;
            }
        }
        else if (VK_F12 == wParam)
        {
            // show or hiden border
            if (!hiden)
            {
                hMenu = GetMenu(hWnd);
                dwStyle = (DWORD)GetWindowLong(hWnd, GWL_STYLE);
                GetWindowRect(hWnd, &border);
                GetClientRectExceptStatusBar(hWnd, &rc);
                ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.left));
                ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.right));
                border.left = rc.left - border.left;
                border.right -= rc.right;
                border.top = rc.top - border.top;
                border.bottom -= rc.bottom;
                SetWindowLong(hWnd, GWL_STYLE, dwStyle & (~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU)));
                SetMenu(hWnd, NULL);
                ShowWindow(_hWndStatus, SW_HIDE);
                SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_DRAWFRAME);
            }
            else
            {
                GetWindowRect(hWnd, &rc);
                SetWindowLong(hWnd, GWL_STYLE, dwStyle);
                SetMenu(hWnd, hMenu);
                ShowWindow(_hWndStatus, SW_SHOW);
                SetWindowPos(hWnd, NULL, rc.left-border.left, rc.top-border.top, 
                    rc.right-rc.left+border.left+border.right, rc.bottom-rc.top+border.top+border.bottom, SWP_DRAWFRAME);
            }
            hiden = !hiden;
        }
        break;
    case WM_HOTKEY:
        if (ID_HOTKEY_SHOW_HIDE_WINDOW == wParam)
        {
            // show or hide window
            static bool isShow = true;
            ShowWindow(hWnd, isShow ? SW_HIDE : SW_SHOW);
            SetForegroundWindow(hWnd);
            isShow = !isShow;
        }
        break;
    case WM_LBUTTONDOWN:
        OnPageDown(hWnd);
        break;
    case WM_RBUTTONDOWN:
        OnPageUp(hWnd);
        break;
    case WM_MOUSEWHEEL:
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
        {
            OnLineUp(hWnd);
        }
        else
        {
            OnLineDown(hWnd);
        }
        break;
    case WM_ERASEBKGND:
        if (!_Text)
            DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_DROPFILES:
        OnDropFiles(hWnd, message, wParam, lParam);
        break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
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
		break;
    case WM_LBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        hWnd = GetDlgItem(hDlg, IDC_STATIC_LINK);
        ClientToScreen(hDlg, &pt);
        GetWindowRect(hWnd, &rc);
        if (PtInRect(&rc, pt))
        {
            ShellExecute(NULL, _T("open"), _T("https://github.com/binbyu/Reader"), NULL, NULL, SW_SHOWNORMAL);
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
    LRESULT res;
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDC_EDIT_LINEGAP, _header->line_gap, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_BORDER, _header->internal_border, FALSE);
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_ENABLE_MC), BM_SETCHECK, _header->enable_click_page ? BST_CHECKED : BST_UNCHECKED, NULL);
        WheelSpeedInit(hDlg);
        // init hotkey
        HotkeyInit(hDlg);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
                GetDlgItemInt(hDlg, IDC_EDIT_LINEGAP, &bResult, FALSE);
                if (!bResult)
                {
                    MessageBox(hDlg, _T("Invalid line gap value!"), _T("Error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                GetDlgItemInt(hDlg, IDC_EDIT_BORDER, &bResult, FALSE);
                if (!bResult)
                {
                    MessageBox(hDlg, _T("Invalid internal border value!"), _T("Error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                // save hot key
                if (!HotkeySave(hDlg))
                {
                    return (INT_PTR)TRUE;
                }
                if (SendMessage(GetDlgItem(hDlg, IDC_COMBO_SPEED), CB_GETCURSEL, 0, NULL) != -1)
                {
                    _header->wheel_speed = SendMessage(GetDlgItem(hDlg, IDC_COMBO_SPEED), CB_GETCURSEL, 0, NULL) + 1;
                }
                res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_ENABLE_MC), BM_GETCHECK, 0, NULL);
                if (res == BST_CHECKED)
                    _header->enable_click_page = 1;
                else
                    _header->enable_click_page = 0;
                value = GetDlgItemInt(hDlg, IDC_EDIT_LINEGAP, &bResult, FALSE);
                if (value != _header->line_gap)
                {
                    _header->line_gap = value;
                    bUpdated = TRUE;
                }
                value = GetDlgItemInt(hDlg, IDC_EDIT_BORDER, &bResult, FALSE);
                if (value != _header->internal_border)
                {
                    _header->internal_border = value;
                    bUpdated = TRUE;
                }
                break;
            case IDCANCEL:
                break;
            default:
                break;
            }

            EndDialog(hDlg, LOWORD(wParam));

            if (bUpdated)
            {
                _PageCache.Reset(GetParent(hDlg));
            }

            return (INT_PTR)TRUE;
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
        progress = (double)(_item->index+_PageCache.GetCurPageSize())*100/_TextLen;
        _stprintf(buf, _T("%0.2f"), progress);
        SetWindowText(hWnd, buf);
        return (INT_PTR)TRUE;

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
                        MessageBox(hDlg, _T("Invalid value!"), _T("Error"), MB_OK|MB_ICONERROR);
                        return (INT_PTR)TRUE;
                    }
                    _item->index = (int)(progress * _TextLen / 100);
                    if (_item->index == _TextLen)
                        _item->index--;
                    _PageCache.Reset(GetParent(hDlg));
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


LRESULT OnCreate(HWND hWnd)
{
    // create status bar
    _hWndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE, _T("Please open a text."), hWnd, IDC_STATUSBAR);
    // register find text dialog
    _uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);
    OnUpdateMenu(hWnd);

    // open the last file
    if (_header->size > 0)
    {
        item_t* item = _Cache.get_item(0);
        OnOpenFile(hWnd, item->file_name);
    }
    return 0;
}

LRESULT OnUpdateMenu(HWND hWnd)
{
    int menu_begin_id = IDM_OPEN_BEGIN;
    HMENU hMenuBar = GetMenu(hWnd);
    HMENU hFile = GetSubMenu(hMenuBar, 0);
    if (hFile)
    {
        DeleteMenu(hMenuBar, 0, MF_BYPOSITION);
        hFile = NULL;
    }
    hFile = CreateMenu();
    AppendMenu(hFile, MF_STRING, IDM_OPEN, _T("&Open"));
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    for (int i=0; i<_header->size; i++)
    {
        item_t* item = _Cache.get_item(i);
        AppendMenu(hFile, MF_STRING, (UINT_PTR)menu_begin_id++, item->file_name);
    }
    if (_header->size > 0)
        AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFile, MF_STRING, IDM_EXIT, _T("E&xit"));
    InsertMenu(hMenuBar, 0, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hFile, L"&File");
    DrawMenuBar(hWnd);

    return 0;
}

LRESULT OnOpenItem(HWND hWnd, int item_id)
{
    if (IsWindow(_hFindDlg))
    {
        DestroyWindow(_hFindDlg);
        _hFindDlg = NULL;
    }
    if (_item && _item->id == item_id)
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
            if (!ReadAllAndDecode(hWnd, item->file_name, &item))
            {
                return 0;
            }

            // open item
            _item = _Cache.open_item(item->id);

            // get chapter
            if (_hThreadChapter)
            {
                TerminateThread(_hThreadChapter, 0);
                CloseHandle(_hThreadChapter);
                _hThreadChapter = NULL;
            }
            _ChapterMap.clear();
            DWORD dwThreadId;
            _hThreadChapter = CreateThread(NULL, 0, ThreadProcChapter, hWnd, CREATE_SUSPENDED, &dwThreadId);
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

    // set text
    _PageCache.SetText(hWnd, _Text, _TextLen, &_item->index, &_header->line_gap, &_header->internal_border);
    
    // update menu
    OnUpdateMenu(hWnd);

    // Update title
    TCHAR szFileName[MAX_PATH] = {0};
    memcpy(szFileName, _item->file_name, sizeof(szFileName));
    if (!_szSrcTitle[0])
        GetWindowText(hWnd, _szSrcTitle, MAX_PATH);
    PathRemoveExtension(szFileName);
    _stprintf(szTitle, _T("%s - %s"), _szSrcTitle, PathFindFileName(szFileName));
    SetWindowText(hWnd, szTitle);

    // repaint
    RECT rc;
    GetClientRectExceptStatusBar(hWnd, &rc);
    InvalidateRect(hWnd, &rc, TRUE);

    if (_hThreadChapter)
        ResumeThread(_hThreadChapter);
	return 0;
}

LRESULT OnOpenFile(HWND hWnd, TCHAR *filename)
{
    item_t* item = NULL;
    TCHAR szFileName[MAX_PATH] = {0};

    memcpy(szFileName, filename, MAX_PATH);

    if (!ReadAllAndDecode(hWnd, szFileName, &item))
    {
        return 0;
    }

    // open item
    _item = _Cache.open_item(item->id);

    // set text
    _PageCache.SetText(hWnd, _Text, _TextLen, &_item->index, &_header->line_gap, &_header->internal_border);

    // get chapter
    if (_hThreadChapter)
    {
        TerminateThread(_hThreadChapter, 0);
        CloseHandle(_hThreadChapter);
        _hThreadChapter = NULL;
    }
    _ChapterMap.clear();
    DWORD dwThreadId;
    _hThreadChapter = CreateThread(NULL, 0, ThreadProcChapter, hWnd, CREATE_SUSPENDED, &dwThreadId);

    // update menu
    OnUpdateMenu(hWnd);

    // Update title
    if (!_szSrcTitle[0])
        GetWindowText(hWnd, _szSrcTitle, MAX_PATH);
    PathRemoveExtension(szFileName);
    _stprintf(szTitle, _T("%s - %s"), _szSrcTitle, PathFindFileName(szFileName));
    SetWindowText(hWnd, szTitle);

    // repaint
    RECT rc;
    GetClientRectExceptStatusBar(hWnd, &rc);
    InvalidateRect(hWnd, &rc, TRUE);

    if (_hThreadChapter)
        ResumeThread(_hThreadChapter);

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
    TCHAR szTitle[MAX_PATH] = {0};
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);  
    ofn.hwndOwner = hWnd;  
    ofn.lpstrFilter = _T("txt(*.txt)\0*.txt\0");
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

    return OnOpenFile(hWnd, szFileName);
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

        if (0 != memcmp(&logFont, &_header->font, sizeof(LOGFONT)))
        {
            memcpy(&_header->font, &logFont, sizeof(LOGFONT));
            bUpdate = TRUE;
        }
        if (bUpdate)
            _PageCache.Reset(hWnd);
    }

    return 0;
}

LRESULT OnSetBkColor(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CHOOSECOLOR cc;                 // common dialog box structure 
    static COLORREF acrCustClr[16]; // array of custom colors 

    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hWnd;
    cc.lpCustColors = (LPDWORD) acrCustClr;
    cc.rgbResult = _header->bk_color;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc))
    {
        if (_header->bk_color != cc.rgbResult)
        {
            _header->bk_color = cc.rgbResult;
            _PageCache.ReDraw(hWnd);
        }
    }
    
    return 0;
}

LRESULT OnRestoreDefault(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    _Cache.default_header();
    SetWindowPos(hWnd, NULL,
        _header->rect.left, _header->rect.top,
        _header->rect.right - _header->rect.left,
        _header->rect.bottom - _header->rect.top,
        SWP_DRAWFRAME);
    _PageCache.Reset(hWnd);
    RegisterHotKey(hWnd, ID_HOTKEY_SHOW_HIDE_WINDOW, _header->hk_show_1 | _header->hk_show_2 | MOD_NOREPEAT, _header->hk_show_3);
    return 0;
}

LRESULT OnPaint(HWND hWnd, HDC hdc)
{
    RECT rc;
    GetClientRectExceptStatusBar(hWnd, &rc);

    // memory dc
    HDC memdc = CreateCompatibleDC(hdc);
    HBITMAP hBmp = CreateCompatibleBitmap(hdc, rc.right-rc.left, rc.bottom-rc.top);
    SelectObject(memdc, hBmp);

    // set font
    HFONT hFont = CreateFontIndirect(&_header->font);
    SelectObject(memdc, hFont);
    SetTextColor(memdc, _header->font_color);

    // set bk color
    HBRUSH hBrush = CreateSolidBrush(_header->bk_color);
    SelectObject(memdc, hBrush);
    FillRect(memdc, &rc, hBrush);
    SetBkMode(memdc, TRANSPARENT);

    _PageCache.DrawPage(memdc);

    BitBlt(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, memdc, rc.left, rc.top, SRCCOPY);

    DeleteObject(hBmp);
    DeleteObject(hFont);
    DeleteObject(hBrush);
    DeleteDC(memdc);
    UpdateProgess();
    return 0;
}

LRESULT OnSize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    GetClientRectExceptStatusBar(hWnd, &rc);
    _PageCache.SetRect(&rc);
    SendMessage(_hWndStatus, message, wParam, lParam);
    return 0;
}

LRESULT OnMove(HWND hWnd)
{
    // save rect to cache file when exit app
    return 0;
}

LRESULT OnPageUp(HWND hWnd)
{
    if (_header->enable_click_page)
        _PageCache.PageUp(hWnd);
    return 0;
}

LRESULT OnPageDown(HWND hWnd)
{
    if (_header->enable_click_page)
        _PageCache.PageDown(hWnd);
    return 0;
}

LRESULT OnLineUp(HWND hWnd)
{
    _PageCache.LineUp(hWnd, _header->wheel_speed);
    return 0;
}

LRESULT OnLineDown(HWND hWnd)
{
    _PageCache.LineDown(hWnd, _header->wheel_speed);
    return 0;
}

LRESULT OnGotoPrevChapter(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!_Text || _item->index == 0)
        return 0;

    std::map<int, int>::reverse_iterator itor;
    for (itor = _ChapterMap.rbegin(); itor != _ChapterMap.rend(); itor++)
    {
        if (itor->second < _item->index)
        {
            _item->index = itor->second;
            _PageCache.Reset(hWnd);
            break;
        }
    }
    return 0;
}

LRESULT OnGotoNextChapter(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!_Text || _TextLen == _item->index+_PageCache.GetCurPageSize())
        return 0;

    std::map<int, int>::iterator itor;
    for (itor = _ChapterMap.begin(); itor != _ChapterMap.end(); itor++)
    {
        if (itor->second > _item->index)
        {
            _item->index = itor->second;
            _PageCache.Reset(hWnd);
            break;
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

    for (UINT i = 0; i < nFileCount; i++)
    {
        ::DragQueryFile(hDropInfo, i, szFileName, sizeof(szFileName));
        dwAttribute = ::GetFileAttributes(szFileName);

        if (dwAttribute & FILE_ATTRIBUTE_DIRECTORY)
        {          
            continue;
        }

        // check is txt file
        if (_tcscmp(PathFindExtension(szFileName), _T(".txt")))
        {
            continue;
        }

        // open file
        OnOpenFile(hWnd, szFileName);

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
        if (!_Text)
            return 0;
        int len = _tcslen(szFindWhat);
        if (fr.Flags & FR_DIALOGTERM)
        {
            // close dlg
        }
        else if (fr.Flags & FR_DOWN) // back search
        {
            for (int i=_item->index+1; i<_TextLen-len+1; i++)
            {
                if (0 == memcmp(szFindWhat, _Text+i, len*sizeof(TCHAR)))
                {
                    _item->index = i;
                    _PageCache.Reset(hWnd);
                    break;
                }
            }
        }
        else // front search
        {
            for (int i=_item->index-3; i>=0; i--)
            {
                if (0 == memcmp(szFindWhat, _Text+i, len*sizeof(TCHAR)))
                {
                    _item->index = i;
                    _PageCache.Reset(hWnd);
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

UINT GetAppVersion(void)
{
    // Not a real version, just used to flag whether you need to update the cache.dat file.
    char version[4] = {'1','3','0','0'};
    UINT ver = 0;

    ver = version[0] << 24 | version[1] << 16 | version[2] << 8 | version[3];
    return ver;
}

BOOL Init(void)
{
    if (!_Cache.init())
    {
        MessageBox(NULL, _T("Init Cache fail."), _T("Error"), MB_OK);
        return FALSE;
    }

    _header = _Cache.get_header();

    // delete not exist items
    std::vector<int> delVec;
    for (int i=0; i<_header->size; i++)
    {
        item_t* item = _Cache.get_item(i);
        if (!PathFileExists(item->file_name))
        {
            delVec.push_back(i);
        }
    }
    for (size_t i=0; i<delVec.size(); i++)
    {
        _Cache.delete_item(delVec[i]);
    }

    return TRUE;
}

void Exit(void)
{
    if (_Text)
        free(_Text);
    
    if (_hThreadChapter)
    {
        TerminateThread(_hThreadChapter, 0);
        CloseHandle(_hThreadChapter);
        _hThreadChapter = NULL;
    }
    _ChapterMap.clear();

    if (!_Cache.exit())
    {
        MessageBox(NULL, _T("Save Cache fail."), _T("Error"), MB_OK);
    }
}

BOOL ReadAllAndDecode(HWND hWnd, TCHAR* szFileName, item_t** item)
{
    // read full file context
    FILE* fp = _tfopen(szFileName, _T("rb"));
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    char* buf = (char*)malloc(len+1);
    if (!buf)
    {
        // Memory is not enough 
        MessageBox(hWnd, _T("Memory is not enough."), _T("Error"), MB_OK);
        return FALSE;
    }
    fseek(fp, 0, SEEK_SET);
    int size = fread(buf, 1, len, fp);
    buf[size] = 0;
    fclose(fp);

    // get md5
    u128_t md5;
    memset(&md5, 0, sizeof(u128_t));
    Utils::get_md5(buf, size, &md5);

    // check cache
    if (NULL == *item)
    {
        *item = _Cache.find_item(&md5, szFileName);
        if (*item) // already exist
        {
            if (_item && (*item)->id == _item->id) // current is opened
            {
                free(buf);
                return FALSE;
            }
        }
        else
        {
            *item = _Cache.new_item(&md5, szFileName);
            _header = _Cache.get_header(); // after realloc the header address has been changed
        }
    }

    // decode
    type_t bom = Unknown;
    char* tmp = buf;
    wchar_t* result = (wchar_t*)buf;
    if (Unknown != (bom = Utils::check_bom(buf, size)))
    {
        if (utf8 == bom)
        {
            tmp += 3;
            size -= 3;
            result = Utils::utf8_to_unicode(tmp, &_TextLen);
            free(buf);
        }
        else if (utf16_le == bom)
        {
            //tmp += 2;
            //size -= 2;
            result = (wchar_t*)tmp;
            _TextLen = size/2;
        }
        else if (utf16_be == bom)
        {
            //tmp += 2;
            //size -= 2;
            tmp = Utils::be_to_le(tmp, size);
            result = (wchar_t*)tmp;
            _TextLen = size/2;
        }
        else if (utf32_le == bom || utf32_be == bom)
        {
            tmp += 4;
            size -= 4;
            // not support
            free(buf);
            MessageBox(hWnd, _T("This file encoding is not support."), _T("Error"), MB_OK);
            return FALSE;
        }
    }
    else if (Utils::is_ascii(buf, size > 1024 ? 1024 : size))
    {
        result = Utils::utf8_to_unicode(tmp, &_TextLen);
        free(buf);
    }
    else if (Utils::is_utf8(buf, size > 1024 ? 1024 : size))
    {
        result = Utils::utf8_to_unicode(tmp, &_TextLen);
        free(buf);
    }
    else
    {
        result = Utils::ansi_to_unicode(tmp, &_TextLen);
        free(buf);
    }
    if (_Text)
    {
        free(_Text);
    }
    _Text = result;
    FormatText();
    return TRUE;
}

void FormatText(void)
{
    TCHAR *buf = NULL;
    int i, index = 0;
    if (!_Text || _TextLen == 0)
        return;

    buf = (TCHAR *)malloc(_TextLen * sizeof(TCHAR));
    for (i = 0; i < _TextLen; i++)
    {
        if (_Text[i] == 0x0D && _Text[i + 1] == 0x0A)
            i++;
        buf[index++] = _Text[i];
    }
#if 0
    buf[index] = 0;
#else
    index--;
#endif
    _TextLen = index;
    memcpy(_Text, buf, _TextLen * sizeof(TCHAR));
    free(buf);
}

void UpdateProgess(void)
{
    static TCHAR progress[32] = {0};
    double dprog = 0.0;
    int nprog = 0;
    if (_item)
    {
        dprog = (double)(_item->index+_PageCache.GetCurPageSize())*100.0/_TextLen;
        nprog = (int)(dprog * 100);
        dprog = (double)nprog / 100.0;
        _stprintf(progress, _T("Progress: %.2f%%"), dprog);
        SendMessage(_hWndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)progress);
    }
    else
    {
        SendMessage(_hWndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
    }
}

BOOL GetClientRectExceptStatusBar(HWND hWnd, RECT* rc)
{
    RECT rect;
    GetClientRect(_hWndStatus, &rect);
    GetClientRect(hWnd, rc);
    if (IsWindowVisible(_hWndStatus))
    {
        rc->bottom -= rect.bottom;
    }
    return TRUE;
}

inline BOOL GetLine(TCHAR* buf, int len, int* line_size)
{
    if (!buf || len <= 0)
        return FALSE;

    for (int i=0; i<len; i++)
    {
        if (buf[i] == 0x0A)
        {
            if (i > 0)
            {
                *line_size = i;
                return TRUE;
            }
        }
        else if (i < len - 1
            && buf[i] == 0x0D && buf[i+1] == 0x0A)
        {
            if (i > 0)
            {
                *line_size = i;
                return TRUE;
            }
        }
    }
    *line_size = len;
    return TRUE;
}

const TCHAR valid_chapter[] = {
    _T(' '), _T('\t'),
    _T('0'), _T('1'), _T('2'), _T('3'), _T('4'),
    _T('5'), _T('6'), _T('7'), _T('8'), _T('9'),
    _T('Áã'), _T('Ò»'), _T('¶þ'), _T('Èý'), _T('ËÄ'),
    _T('Îå'), _T('Áù'), _T('Æß'), _T('°Ë'), _T('¾Å'), _T('Ê®'),
    _T('Ò¼'), _T('·¡'), _T('Èþ'), _T('ËÁ'),
    _T('Îé'), _T('Â½'), _T('Æâ'), _T('°Æ'), _T('¾Á'), _T('Ê°'),
};

inline BOOL IsChapter(TCHAR* buf, int len)
{
    BOOL bFound = FALSE;
    if (!buf || len <= 0)
        return FALSE;

    for (int i=0; i<len; i++)
    {
        bFound = FALSE;
        for (int j=0; j<sizeof(valid_chapter); j++)
        {
            if (buf[i] == valid_chapter[j])
            {
                bFound = TRUE;
                break;
            }
        }
        if (!bFound)
        {
            return FALSE;
        }
    }
    return TRUE;
}

DWORD WINAPI ThreadProcChapter(LPVOID lpParam)
{
    HWND hWnd = (HWND)lpParam;
    TCHAR title[MAX_CHAPTER_LENGTH] = {0};
    int title_len = 0;
    TCHAR* index = _Text;
    int line_size;
    int menu_begin_id = IDM_CHAPTER_BEGIN;

    HMENU hMenuBar = GetMenu(hWnd);
    HMENU hView = CreateMenu();

    while (TRUE)
    {
        if (!GetLine(index, _TextLen-(index-_Text), &line_size))
        {
            break;
        }

        // check format
        BOOL bFound = FALSE;
        int idx_1 = -1, idx_2 = -1;
        for (int i=0; i<line_size; i++)
        {
            if (index[i] == _T('µÚ'))
            {
                idx_1 = i;
            }
            if (idx_1>-1
                && ((line_size > i+1 && index[i+1] == _T(' ') 
                || index[i+1] == _T('\t'))
                || line_size <= i+1))
            {
                if (index[i] == _T('¾í')
                    || index[i] == _T('ÕÂ')
                    || index[i] == _T('²¿')
                    || index[i] == _T('½Ú'))
                {
                    idx_2 = i;
                    bFound = TRUE;
                    break;
                }
            }
        }
        if (bFound && IsChapter(index+idx_1+1, idx_2-idx_1-1))
        {
            title_len = line_size-idx_1 < MAX_CHAPTER_LENGTH ? line_size-idx_1 : MAX_CHAPTER_LENGTH-1;
            memcpy(title, index+idx_1, title_len*sizeof(TCHAR));
            title[title_len] = 0;
            // update menu
            AppendMenu(hView, MF_STRING, menu_begin_id, title);
            _ChapterMap.insert(std::make_pair(menu_begin_id++, idx_1+(index-_Text)));
        }

        // set index
        index += line_size;
    }

    DeleteMenu(hMenuBar, 1, MF_BYPOSITION);
    InsertMenu(hMenuBar, 1, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hView, L"&View");
    DrawMenuBar(hWnd);
    return 0;
}

void WheelSpeedInit(HWND hDlg)
{
    int i;
    HWND hWnd = NULL;
    TCHAR buf[8] = {0};

    hWnd = GetDlgItem(hDlg, IDC_COMBO_SPEED);
    for (i=1; i<=_PageCache.GetOnePageLineCount(); i++)
    {
        _itot(i, buf, 10);
        SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)buf);
    }
    SendMessage(hWnd, CB_SETCURSEL, _header->wheel_speed - 1, NULL);
}

void HotkeyInit(HWND hDlg)
{
    HWND hWnd = NULL;
    TCHAR buf[2] = {0};


    // IDC_COMBO_SHOW_1
    hWnd = GetDlgItem(hDlg, IDC_COMBO_SHOW_1);
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T(""));
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Ctrl"));
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Alt"));
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Shift"));
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Win"));
    SendMessage(hWnd, CB_SETCURSEL, HotKeyMap_KeyToIndex(_header->hk_show_1, IDC_COMBO_SHOW_1), NULL);

    // IDC_COMBO_SHOW_2
    hWnd = GetDlgItem(hDlg, IDC_COMBO_SHOW_2);
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Ctrl"));
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Alt"));
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Shift"));
    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Win"));
    SendMessage(hWnd, CB_SETCURSEL, HotKeyMap_KeyToIndex(_header->hk_show_2, IDC_COMBO_SHOW_2), NULL);

    // IDC_COMBO_SHOW_3
    hWnd = GetDlgItem(hDlg, IDC_COMBO_SHOW_3);
    for (int i='A'; i<='Z'; i++)
    {
        buf[0] = i;
        SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)buf);
    }
    SendMessage(hWnd, CB_SETCURSEL, HotKeyMap_KeyToIndex(_header->hk_show_3, IDC_COMBO_SHOW_3), NULL);
}

BOOL HotkeySave(HWND hDlg)
{
    int vk_1, vk_2, vk_3;
    TCHAR msg[256] = {0};

    vk_1 = HotKeyMap_IndexToKey(SendMessage(GetDlgItem(hDlg, IDC_COMBO_SHOW_1), CB_GETCURSEL, 0, NULL), IDC_COMBO_SHOW_1);
    vk_2 = HotKeyMap_IndexToKey(SendMessage(GetDlgItem(hDlg, IDC_COMBO_SHOW_2), CB_GETCURSEL, 0, NULL), IDC_COMBO_SHOW_2);
    vk_3 = HotKeyMap_IndexToKey(SendMessage(GetDlgItem(hDlg, IDC_COMBO_SHOW_3), CB_GETCURSEL, 0, NULL), IDC_COMBO_SHOW_3);
    
    if (vk_1 != _header->hk_show_1 || vk_2 != _header->hk_show_2 || vk_3 != _header->hk_show_3)
    {
        if (!RegisterHotKey(GetParent(hDlg), ID_HOTKEY_SHOW_HIDE_WINDOW, vk_1 | vk_2 | MOD_NOREPEAT, vk_3))
        {
            if (vk_1 == 0)
            {
                _stprintf(msg, _T("[%s+%s]\r\nis invalid or occupied.\r\nPlease choose another key.")
                    ,HotKeyMap_KeyToString(vk_2, IDC_COMBO_SHOW_2)
                    ,HotKeyMap_KeyToString(vk_3, IDC_COMBO_SHOW_3));
            }
            else
            {
                _stprintf(msg, _T("[%s+%s+%s]\r\nis invalid or occupied.\r\nPlease choose another key.")
                    ,HotKeyMap_KeyToString(vk_1, IDC_COMBO_SHOW_1)
                    ,HotKeyMap_KeyToString(vk_2, IDC_COMBO_SHOW_2)
                    ,HotKeyMap_KeyToString(vk_3, IDC_COMBO_SHOW_3));
            }
            MessageBox(hDlg, msg, _T("Error"), MB_OK|MB_ICONERROR);

            return FALSE;
        }
    }
    
    _header->hk_show_1 = vk_1;
    _header->hk_show_2 = vk_2;
    _header->hk_show_3 = vk_3;
    return TRUE;
}

static int s_hk_map_1[5] = {
    0, MOD_CONTROL, MOD_ALT, MOD_SHIFT, MOD_WIN
};

static int s_hk_map_2[4] = {
    MOD_CONTROL, MOD_ALT, MOD_SHIFT, MOD_WIN
};

static int s_hk_map_3[26] = {
    'A', 'B', 'C', 'D', 'E',
    'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y',
    'Z'
};
int HotKeyMap_IndexToKey(int index, int cid)
{
    if (index < 0)
        return 0;

    if (cid == IDC_COMBO_SHOW_1)
    {
        return s_hk_map_1[index];
    }
    else if (cid == IDC_COMBO_SHOW_2)
    {
        return s_hk_map_2[index];
    }
    else
    {
        return s_hk_map_3[index];
    }
}

int HotKeyMap_KeyToIndex(int key, int cid)
{
    if (cid == IDC_COMBO_SHOW_1)
    {
        for (int i=0; i<5; i++)
        {
            if (key == s_hk_map_1[i])
            {
                return i;
            }
        }
    }
    else if (cid == IDC_COMBO_SHOW_2)
    {
        for (int i=0; i<4; i++)
        {
            if (key == s_hk_map_2[i])
            {
                return i;
            }
        }
    }
    else
    {
        for (int i=0; i<26; i++)
        {
            if (key == s_hk_map_3[i])
            {
                return i;
            }
        }
    }
    return -1;
}

TCHAR* HotKeyMap_KeyToString(int key, int cid)
{
    static TCHAR buf1[8] = {0};
    static TCHAR buf2[8] = {0};
    static TCHAR buf3[8] = {0};


    if (cid == IDC_COMBO_SHOW_1)
    {
        switch (key)
        {
        case 0:
            memset(buf1, 0, sizeof(TCHAR)*8);
            break;
        case MOD_CONTROL:
            _tcscpy(buf1, _T("Ctrl"));
            break;
        case MOD_ALT:
            _tcscpy(buf1, _T("Alt"));
            break;
        case MOD_SHIFT:
            _tcscpy(buf1, _T("Shift"));
            break;
        case MOD_WIN:
            _tcscpy(buf1, _T("Win"));
            break;
        }
        return buf1;
    }
    else if (cid == IDC_COMBO_SHOW_2)
    {
        switch (key)
        {
        case MOD_CONTROL:
            _tcscpy(buf2, _T("Ctrl"));
            break;
        case MOD_ALT:
            _tcscpy(buf2, _T("Alt"));
            break;
        case MOD_SHIFT:
            _tcscpy(buf2, _T("Shift"));
            break;
        case MOD_WIN:
            _tcscpy(buf2, _T("Win"));
            break;
        }
        return buf2;
    }
    else
    {
        memset(buf3, 0, sizeof(TCHAR)*8);
        buf3[0] = key;
        return buf3;
    }
}