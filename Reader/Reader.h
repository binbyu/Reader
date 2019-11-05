#pragma once

#include "resource.h"
#include "Cache.h"
#include "Utils.h"
#include "MiniStack.h"

#define ID_HOTKEY_SHOW_HIDE_WINDOW  100
#define ID_HOTKEY_TOP_WINDOW        101



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
LRESULT             OnGotoPrevChapter(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnGotoNextChapter(HWND, UINT, WPARAM, LPARAM);
UINT                GetAppVersion(void);
BOOL                Init(void);
void                Exit(void);
LONG                GetFontHeight(HWND, HDC);
LONG                CalcCount(HWND, HDC, TCHAR*, UINT, BOOL);
LONG                ReCalcCount(HWND, HDC, TCHAR*, UINT, BOOL);
BOOL                ReadAllAndDecode(HWND, TCHAR*, item_t**);
VOID                UpdateProgess(void);
BOOL                GetClientRectExceptStatusBar(HWND, RECT*);
DWORD WINAPI        ThreadProcChapter(LPVOID);
void                HotkeyInit(HWND);
BOOL                HotkeySave(HWND);
int                 HotKeyMap_IndexToKey(int, int);
int                 HotKeyMap_KeyToIndex(int, int);
TCHAR*              HotKeyMap_KeyToString(int, int);