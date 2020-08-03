#ifndef __READER_H__
#define __READER_H__


#include "resource.h"
#include "Cache.h"
#include "Utils.h"
#if ENABLE_NETWORK
#include "Upgrade.h"
#endif
#include "Book.h"
#include <map>
#include <shellapi.h>

typedef struct loading_data_t
{
    BOOL enable;
    Image *image;
    PropertyItem *item;
    UINT frameCount;
    UINT frameIndex;
    HGLOBAL hMemory;
} loading_data_t;


Cache               _Cache(CACHE_FILE_NAME);
header_t*           _header                 = NULL;
item_t*             _item                   = NULL;
HWND                _hWndStatus             = NULL;
HWND                _hFindDlg               = NULL;
HWND                _hTreeView              = NULL;
HWND                _hTreeMark              = NULL;
UINT                _uFindReplaceMsg        = 0;
window_info_t       _WndInfo                = { 0 };
BOOL                _IsAutoPage             = FALSE;
#if ENABLE_NETWORK
Upgrade             _Upgrade;
#endif
Book *              _Book                   = NULL;
loading_data_t *    _loading                = NULL;
BOOL                _bShowText              = TRUE;
HHOOK               _hMouseHook             = NULL;
HWND                _hWnd                   = NULL;
NOTIFYICONDATA      _nid                    = { 0 };
BYTE                _textAlpha              = 0xFF;


LRESULT             OnCreate(HWND);
LRESULT             OnUpdateMenu(HWND);
LRESULT             OnOpenItem(HWND, int);
LRESULT             OnClearFileList(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnSetFont(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnSetBkColor(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnRestoreDefault(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnPaint(HWND, HDC);
void                OnDraw(HWND);
void                ResetLayerd(HWND);
LRESULT             OnSize(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnMove(HWND);
// WM_KEYWORD begin
LRESULT             OnHideWin(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnHideBorder(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnFullScreen(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnTopmost(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnOpenFile(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnAddMark(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnAutoPage(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnSearch(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnJump(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnPageUp(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnPageDown(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnLineUp(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnLineDown(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnChapterUp(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnChapterDown(HWND, UINT, WPARAM, LPARAM);
// WM_KEYWORD end
LRESULT             OnDropFiles(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnFindText(HWND, UINT, WPARAM, LPARAM);
LRESULT             OnUpdateChapters(HWND);
LRESULT             OnUpdateBookMark(HWND);
LRESULT             OnOpenBookResult(HWND, BOOL);
LRESULT CALLBACK    MouseProc(int, WPARAM, LPARAM);
void                ShowHideWindow(HWND);
void                OnOpenBook(HWND, TCHAR *);
UINT                GetCacheVersion(void);
BOOL                Init(void);
void                Exit(void);
void                UpdateProgess(void);
BOOL                GetClientRectExceptStatusBar(HWND, RECT*);
void                WheelSpeedInit(HWND);
Bitmap*             LoadBGImage(int, int, BYTE alpha=0xFF);
BOOL                FileExists(TCHAR *);
void                StartAutoPage(HWND);
void                StopAutoPage(HWND);
void                PauseAutoPage(HWND);
void                ResumeAutoPage(HWND);
#if ENABLE_NETWORK
void                CheckUpgrade(HWND);
bool                UpgradeCallback(void *, json_item_data_t *);
#endif
bool                PlayLoadingImage(HWND);
bool                StopLoadingImage(HWND);
ULONGLONG           GetDllVersion(LPCTSTR lpszDllName);
BOOL CALLBACK       EnumWindowsProc(HWND hWnd, LPARAM lParam);
void                ShowInTaskbar(HWND hWnd, BOOL bShow);
void                ShowSysTray(HWND hWnd, BOOL bShow);
HBITMAP             CreateAlphaTextBitmap(HFONT inFont, COLORREF inColour, int width, int height);


#endif
