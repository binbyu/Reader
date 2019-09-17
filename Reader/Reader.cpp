// Reader.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Reader.h"
#include "Cache.h"
#include "Utils.h"
#include "MiniStack.h"
#include <stdio.h>
#include <shlwapi.h>
#include <CommDlg.h>
#include <commctrl.h>
#include <map>
#include <vector>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")


#define MAX_LOADSTRING              100
#define FONT_LINE_GAP               5
#define INTERNAL_BORDER             0  // left and right border

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

Cache               _Cache(CACHE_FILE_NAME);
header_t*           _header         = NULL;
item_t*             _item           = NULL;
TCHAR*              _Text           = NULL;
int                 _TextLen        = 0;
int                 _CurPageCount   = 0;
BOOL                _bPrevOrNext    = FALSE;
HWND                _hWndStatus     = NULL;
HWND                _hFindDlg       = NULL;
UINT                _uFindReplaceMsg=0;
HANDLE              _hThreadChapter = NULL;
std::map<int, int>  _ChapterMap;
TCHAR               _szSrcTitle[MAX_PATH] = {0};

LRESULT             OnCreate(HWND);
LRESULT             OnUpdateMenu(HWND);
LRESULT             OnOpenItem(HWND, int);
LRESULT             OnOpenFile(HWND, TCHAR*);
LRESULT             OnOpenFile(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnSetFont(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnSetBkColor(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnRestoreDefault(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnPaint(HWND, HDC);
LRESULT             OnSize(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnMove(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnPrevPage(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnNextPage(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnFindText(HWND, UINT, WPARAM, LPARAM);
BOOL                Init();
void                Exit();
LONG                GetFontHeight(HWND, HDC);
LONG                CalcCount(HWND, HDC, TCHAR*, UINT, BOOL);
LONG                ReCalcCount(HWND, HDC, TCHAR*, UINT, BOOL);
BOOL                ReadAllAndDecode(HWND, TCHAR*, item_t**);
VOID                UpdateProgess();
BOOL                GetClientRectExceptStatusBar(HWND, RECT*);
DWORD WINAPI        ThreadProcChapter(LPVOID);


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

	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
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

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
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
                _Cache.reset_page_info(_item->id);
                RECT rc;
                GetClientRectExceptStatusBar(hWnd, &rc);
                InvalidateRect(hWnd, &rc, TRUE);
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
        case IDM_DEFAULT:
            OnRestoreDefault(hWnd, message, wParam, lParam);
            break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
    case WM_CREATE:
        OnCreate(hWnd);
        break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
        OnPaint(hWnd, hdc);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
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
        OnMove(hWnd, message, wParam, lParam);
        break;
    case WM_KEYDOWN:
        if (VK_LEFT == wParam)
        {
            OnPrevPage(hWnd, message, wParam, lParam);
        }
        else if (VK_RIGHT == wParam)
        {
            OnNextPage(hWnd, message, wParam, lParam);
        }
        else if ('F' == wParam)
        {
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                OnFindText(hWnd, message, wParam, lParam);
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
    case WM_LBUTTONDOWN:
        OnNextPage(hWnd, message, wParam, lParam);
        break;
    case WM_RBUTTONDOWN:
        OnPrevPage(hWnd, message, wParam, lParam);
        break;
    case WM_MOUSEWHEEL:
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
        {
            OnPrevPage(hWnd, message, wParam, lParam);
        }
        else
        {
            OnNextPage(hWnd, message, wParam, lParam);
        }
        break;
    case WM_ERASEBKGND:
        if (!_Text)
            DefWindowProc(hWnd, message, wParam, lParam);
        break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
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
        if (i == 0)
        {
            OnOpenFile(hWnd, item->file_name);
        }
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

LRESULT OnOpenFile(HWND hWnd, TCHAR *szFileName)
{
    item_t* item = NULL;
    if (!ReadAllAndDecode(hWnd, szFileName, &item))
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
        _header->font_color = cf.rgbColors;
        memcpy(&_header->font, &logFont, sizeof(LOGFONT));

        if (0 != memcmp(&logFont, &_header->font, sizeof(LONG)*5))
        {
            _Cache.reset_page_info();
        }

        RECT rc;
        GetClientRectExceptStatusBar(hWnd, &rc);
        InvalidateRect(hWnd, &rc, TRUE);
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
            RECT rc;
            GetClientRectExceptStatusBar(hWnd, &rc);
            InvalidateRect(hWnd, &rc, TRUE);
        }
    }
    
    return 0;
}

LRESULT OnRestoreDefault(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    _Cache.default_header();
    _Cache.reset_page_info();
    SetWindowPos(hWnd, NULL,
        _header->rect.left, _header->rect.top,
        _header->rect.right - _header->rect.left,
        _header->rect.bottom - _header->rect.top,
        SWP_DRAWFRAME);
    RECT rc;
    GetClientRectExceptStatusBar(hWnd, &rc);
    InvalidateRect(hWnd, &rc, TRUE);
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

    if (!_Text)
        return 0;

    // prev page
    if (_bPrevOrNext)
    {
        MiniStack pageInfo(&_item->page_info);
        // using cache page
        if (pageInfo.size() > 0)
        {
            _item->index = pageInfo.top();
            pageInfo.pop();
            _CurPageCount = CalcCount(hWnd, memdc, _Text+_item->index, _TextLen-_item->index, TRUE);
        }
        else
        {
            int count = 0;
            int temp = 0;
            count = ReCalcCount(hWnd, memdc, _Text+_item->index-1, _item->index, FALSE);
            if (_item->index <= count)
            {
                _item->index = 0;
            }
            else
            {
                //try to draw
                temp = CalcCount(hWnd, memdc, _Text+_item->index-count, _TextLen-_item->index+count, FALSE);
                if (temp < count)
                {
                    count = temp;
                }
                _item->index -= count;
            }
            _CurPageCount = CalcCount(hWnd, memdc, _Text+_item->index, _item->index == 0 ? _TextLen : count, TRUE);
        }
        _bPrevOrNext = FALSE;
    }
    // next page
    else
    {
        _CurPageCount = CalcCount(hWnd, memdc, _Text+_item->index, _TextLen-_item->index, TRUE);
    }

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
    GetWindowRect(hWnd, &rc);
    if (rc.right - rc.left != _header->rect.right - _header->rect.left
        || rc.bottom - rc.top != _header->rect.bottom - _header->rect.top)
    {
        //_header->rect.left = rc.left;
        //_header->rect.right = rc.right;
        //_header->rect.top = rc.top;
        //_header->rect.bottom = rc.bottom;
        _Cache.reset_page_info();
    }
    SendMessage(_hWndStatus, message, wParam, lParam);
    return 0;
}

LRESULT OnMove(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //RECT rc;
    //GetWindowRect(hWnd, &rc);
    //if (rc.left != _header->rect.left
    //    || rc.top != _header->rect.top)
    //{
    //    _header->rect.left = rc.left;
    //    _header->rect.right = rc.right;
    //    _header->rect.top = rc.top;
    //    _header->rect.bottom = rc.bottom;
    //}
    return 0;
}

LRESULT OnPrevPage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!_Text || _item->index == 0)
        return 0;
    _bPrevOrNext = TRUE;
    RECT rc;
    GetClientRectExceptStatusBar(hWnd, &rc);
    InvalidateRect(hWnd, &rc, TRUE);
    return 0;
}

LRESULT OnNextPage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!_Text || _TextLen == _item->index+_CurPageCount)
        return 0;
    MiniStack pageInfo(&_item->page_info);
    pageInfo.push(_item->index);
    _item->index += _CurPageCount;
    RECT rc;
    GetClientRectExceptStatusBar(hWnd, &rc);
    InvalidateRect(hWnd, &rc, TRUE);
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
                    _Cache.reset_page_info(_item->id);
                    RECT rc;
                    GetClientRectExceptStatusBar(hWnd, &rc);
                    InvalidateRect(hWnd, &rc, TRUE);
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
                    _Cache.reset_page_info(_item->id);
                    RECT rc;
                    GetClientRectExceptStatusBar(hWnd, &rc);
                    InvalidateRect(hWnd, &rc, TRUE);
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

BOOL Init()
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
    for (int i=0; i<delVec.size(); i++)
    {
        _Cache.delete_item(delVec[i]);
    }

    return TRUE;
}

void Exit()
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

LONG GetFontHeight(HWND hWnd, HDC hdc)
{
    SIZE sz = {0};
    LONG fontHeight = 20;
    GetTextExtentPoint32(hdc, _T("AaBbYyZz"), 8, &sz);
    fontHeight = sz.cy + FONT_LINE_GAP;
    return fontHeight;
}

LONG CalcCount(HWND hWnd, HDC hdc, TCHAR* data, UINT size, BOOL isDraw)
{
    RECT rc,rect;
    GetClientRectExceptStatusBar(hWnd, &rc);
    GetClientRectExceptStatusBar(hWnd, &rect);
    rect.left+=INTERNAL_BORDER;
    rect.top+=INTERNAL_BORDER;
    LONG fontHeight = GetFontHeight(hWnd, hdc);
    LONG maxLine = (rc.bottom+FONT_LINE_GAP-2*INTERNAL_BORDER)/fontHeight;
    LONG index = 0;
    LONG lastIndex = index;
    while (maxLine && index<size)
    {
        // new line
        if (data[index] == 0x0A)
        {
            index++;
            maxLine--;
            if (isDraw)
                DrawText(hdc, data+lastIndex, index-lastIndex, &rect, DT_LEFT);
            lastIndex = index;
            rect.top+=fontHeight;
            continue;
        }
        else if (data[index] == 0x0D && data[index+1] == 0x0A)
        {
            index+=2;
            maxLine--;
            if (isDraw)
                DrawText(hdc, data+lastIndex, index-lastIndex, &rect, DT_LEFT);
            lastIndex = index;
            rect.top+=fontHeight;
            continue;
        }
        // calc char width
        LONG width = INTERNAL_BORDER*2;
        SIZE sz;
        while (width < rc.right && index<size)
        {
            // new line
            if (data[index] == 0x0A)
            {
                index++;
                break;
            }
            else if (data[index] == 0x0D && data[index+1] == 0x0A)
            {
                index+=2;
                break;
            }

            GetTextExtentPoint32(hdc, data+index, 1, &sz);
            width += sz.cx;
            index++;
        }
        if (width > rc.right)
            index--;
        maxLine--;
        if (isDraw)
            DrawText(hdc, data+lastIndex, index-lastIndex, &rect, DT_LEFT);
        lastIndex = index;
        rect.top+=fontHeight;
    }
    return index;
}


LONG ReCalcCount(HWND hWnd, HDC hdc, TCHAR* data, UINT size, BOOL isDraw)
{
    RECT rc,rect;
    GetClientRectExceptStatusBar(hWnd, &rc);
    GetClientRectExceptStatusBar(hWnd, &rect);
    rect.left+=INTERNAL_BORDER;
    rect.top+=INTERNAL_BORDER;
    LONG fontHeight = GetFontHeight(hWnd, hdc);
    LONG maxLine = (rc.bottom+FONT_LINE_GAP-2*INTERNAL_BORDER)/fontHeight;
    LONG index = 0;
    LONG lastIndex = index;
    while (maxLine && size + index > 0)
    {
        // new line
        if (data[index] == 0x0A && size + index > 1 && data[index-1] == 0x0D)
        {
            index-=2;
            maxLine--;
            rect.top = fontHeight*maxLine;
            if (isDraw)
                DrawText(hdc, data+index+1, lastIndex-index, &rect, DT_LEFT);
            lastIndex = index;
            continue;
        }
        else if (data[index] == 0x0A)
        {
            index--;
            maxLine--;
            rect.top = fontHeight*maxLine;
            if (isDraw)
                DrawText(hdc, data+index+1, lastIndex-index, &rect, DT_LEFT);
            lastIndex = index;
            continue;
        }
        // calc char width
        LONG width = INTERNAL_BORDER*2;
        SIZE sz;
        while (width < rc.right && size + index > 0)
        {
            // new line
            if (data[index] == 0x0A && size + index > 1 && data[index-1] == 0x0D)
            {
                index-=2;
                break;
            }
            else if (data[index] == 0x0A)
            {
                index--;
                break;
            }
            GetTextExtentPoint32(hdc, data+index, 1, &sz);
            width += sz.cx;
            index--;
        }
        if (width > rc.right)
            index++;
        maxLine--;
        rect.top = fontHeight*maxLine;
        if (isDraw)
            DrawText(hdc, data+index+1, lastIndex-index, &rect, DT_LEFT);
        lastIndex = index;
    }
    return -index;
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
    return TRUE;
}

VOID UpdateProgess()
{
    static TCHAR progress[32] = {0};
    if (_item)
    {
        _stprintf(progress, _T("Progress: %6.2f%%"), ((float)(_item->index+_CurPageCount)*100/_TextLen));
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
            if (idx_1>-1 && index[i] == _T('¾í') 
                && ((line_size > i+1 && index[i+1] == _T(' ') 
                || index[i+1] == _T('\t'))
                || line_size <= i+1))
            {
                idx_2 = i;
                bFound = TRUE;
                break;
            }
            if (idx_1>-1 && index[i] == _T('ÕÂ')
                && ((line_size > i+1 && (index[i+1] == _T(' ') 
                || index[i+1] == _T('\t')))
                || line_size <= i+1))
            {
                idx_2 = i;
                bFound = TRUE;
                break;
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