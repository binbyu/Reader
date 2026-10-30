#pragma once
#include "Window.h"
#include "IBook.h"
#include "IRenderer.h"
#include "ILayout.h"
#include "Settings.h"
#include "BookSession.h"
#include "EditController.h"

class MainWin : public Window
{
    friend EditController;
public:
    MainWin(HINSTANCE hInstance);
    ~MainWin();

    HWND hFindDlg();
    static const wchar_t* className(HINSTANCE hInstance);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    virtual void prevCreate();
    virtual void postCreate();
    static const WinConfig& config(HINSTANCE hInstance);
    using  KeyProc = std::function<LRESULT(UINT message, WPARAM wParam, LPARAM lParam)>;
    KeyProc keyProc(KeyId id);

protected:
    LRESULT onCreate();
    LRESULT onDestroy();
    LRESULT onPaint(HDC hdc);
    LRESULT onNcHitTest(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onClose(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onSize(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onMove(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onNotify(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onContextMenu(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onMenuRButtonUp(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onKeyDown(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onMouseDown(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onMouseUp(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onMouseWheel(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onTimer(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onDropFiles(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onDpiChanged(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onCopyData(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onHotKey(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onSysTray(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onOpenDisplaySettingDlg(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onOpenBasicSettingDlg(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onOpenAdvanceSettingDlg(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onOpenKeySettingDlg(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onOpenTagSettingDlg(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onOpenOnlineSearchDlg(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onOpenWebDlg(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onOpenNetworkSettingDlg(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onOpenAboutDlg(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onRestoreDefault(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onClear(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onView(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onMark(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onSaveSettingResult(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onNewVersion(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onBookStateChanged(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onBookEvent(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onRedraw(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onUpdateBooks(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onUpdateChapters(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onUpdateMarks(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onHideWindow(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onHideBorder(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onFullScreen(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onTopMost(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onOpenFile(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onAddMark(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onAutoPage(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onSearch(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onJump(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onEditMode(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onPageUp(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onPageDown(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onLineUp(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onLineDown(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onChapterUp(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onChapterDown(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onFontZoomIn(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onFontZoomOut(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onRefreshChapter(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onRefreshContent(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT onMouseHide(UINT message, WPARAM wParam, LPARAM lParam);

protected:
    void playLoading();
    void stopLoading();
    void startAutoPage();
    void stopAutoPage();
    void pauseAutoPage();
    void resumeAutoPage();
    void applyAutoPageTimer();
    void openFindDialog();
    void closeFindDialog();

protected:
    void  minToTray(bool bShow);
    void  hideTaskbar(bool bHide);
    void  registerHotKey(bool bReg);
    void  initTimers();
    void  redrawWindow();
    void  showWindow(bool bShow);
    void  saveWindowPlacement();
    void  switchDisplayMode(DisplayMode mode);
    RECT  drawTextRect();
    RECT  calcNormalRect();
    RECT  calcBorderlessRect();
    int   menuHeight(int width);
    void  initHttps();
    void  setHttpsParam();
    void  deleteMenu(HMENU hMenu, UINT id, BOOL separator = FALSE);
    void  enableMenu(HMENU hMenu, BOOL enabled);
    void  updateTitle();
    void  updateStatusBar();
    void  updateTreeViewFont();
    DWORD toModifiers(WPARAM wParam);
    void  unloadBgImage();
    Image* loadBgImage(int w, int h, BYTE alpha);

protected:
    BookSession*    m_session;
    Data&           m_data;
    HMENU           m_hMenu;
    HWND            m_hStatusBar;
    HWND            m_hTreeView;
    HWND            m_hTreeMark;
    EditController* m_hEditCtrl;
    HWND            m_hFindDlg;
    UINT            m_uFindMsgId;
    HBITMAP         m_hNewBitmap;
    HHOOK           m_hMouseHook;
    HHOOK           m_hKeyboardHook;
    Image*          m_imgLoading;
    Image*          m_imgBackground;
    bool            m_isSwitching;
    bool            m_isCreating;
    bool            m_autoPageEnabled;
    bool            m_autoPagePaused;
    NOTIFYICONDATA  m_nid;
};