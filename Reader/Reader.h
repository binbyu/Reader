#pragma once

#include "resource.h"
#include "Cache.h"
#include "Utils.h"
#include "PageCache.h"
#include <map>

#define ID_HOTKEY_SHOW_HIDE_WINDOW  100
#define ID_HOTKEY_TOP_WINDOW        101
#define IDT_PAGE_TIMER              102


PageCache           _PageCache;
Cache               _Cache(CACHE_FILE_NAME);
header_t*           _header                 = NULL;
item_t*             _item                   = NULL;
TCHAR*              _Text                   = NULL;
int                 _TextLen                = 0;
HWND                _hWndStatus             = NULL;
HWND                _hFindDlg               = NULL;
UINT                _uFindReplaceMsg        = 0;
HANDLE              _hThreadChapter         = NULL;
std::map<int, int>  _ChapterMap;
TCHAR               _szSrcTitle[MAX_PATH]   = {0};
window_info_t       _WndInfo                = {0};
BOOL                _IsAutoPage             = FALSE;


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
LRESULT             OnMove(HWND);
LRESULT             OnPageUp(HWND);
LRESULT             OnPageDown(HWND);
LRESULT             OnLineUp(HWND);
LRESULT             OnLineDown(HWND);
LRESULT             OnFindText(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnGotoPrevChapter(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnGotoNextChapter(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnDropFiles(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnHideBorder(HWND);
LRESULT             OnFullScreen(HWND);
UINT                GetAppVersion(void);
BOOL                Init(void);
void                Exit(void);
BOOL                ReadAllAndDecode(HWND, TCHAR*, item_t**);
void                FormatText(void);
void                UpdateProgess(void);
BOOL                GetClientRectExceptStatusBar(HWND, RECT*);
DWORD WINAPI        ThreadProcChapter(LPVOID);
void                WheelSpeedInit(HWND hDlg);
void                HotkeyInit(HWND);
BOOL                HotkeySave(HWND);
int                 HotKeyMap_IndexToKey(int, int);
int                 HotKeyMap_KeyToIndex(int, int);
TCHAR*              HotKeyMap_KeyToString(int, int);
Bitmap*             LoadBGImage(int, int);
BOOL                FileExists(TCHAR *file);
void                StartAutoPage(HWND);
void                StopAutoPage(HWND);
void                PauseAutoPage(HWND);
void                ResumeAutoPage(HWND);
