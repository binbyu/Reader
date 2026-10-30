#include "MainWin.h"
#include "resource.h"
#include "Logger.h"
#include "Utils.h"
#include "DpiAwareness.h"
#include "DisplaySettingDlg.h"
#include "BasicSettingDlg.h"
#include "AdvanceSettingDlg.h"
#include "KeySettingDlg.h"
#include "TagMgrDlg.h"
#include "UpgradeDlg.h"
#include "OnlineSearchDlg.h"
#include "WebDlg.h"
#include "NetworkSettingDlg.h"
#include "AboutDlg.h"
#include "JumpProgressDlg.h"
#include "conv.h"
#ifdef ENABLE_NETWORK
#include "https.h"
#endif

MainWin::MainWin(HINSTANCE hInstance)
    : Window(hInstance, config(hInstance), nullptr)
    , m_session(nullptr)
    , m_data(Settings::instance().data())
    , m_hMenu(nullptr)
    , m_hStatusBar(nullptr)
    , m_hTreeView(nullptr)
    , m_hTreeMark(nullptr)
    , m_hEditCtrl(nullptr)
    , m_hFindDlg(nullptr)
    , m_uFindMsgId(-1)
    , m_hNewBitmap(nullptr)
    , m_hMouseHook(nullptr)
    , m_hKeyboardHook(nullptr)
    , m_imgLoading(nullptr)
    , m_imgBackground(nullptr)
    , m_isSwitching(false)
    , m_isCreating(false)
    , m_autoPageEnabled(false)
    , m_autoPagePaused(false)
{
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATA);

    create();
}

MainWin::~MainWin()
{
    if (m_hNewBitmap)
    {
        DeleteObject(m_hNewBitmap);
        m_hNewBitmap = nullptr;
    }
    if (m_imgLoading)
    {
        delete m_imgLoading;
        m_imgLoading = nullptr;
    }
    unloadBgImage();
    if (m_session)
    {
        delete m_session;
        m_session = nullptr;
    }
    IRenderer::instance()->cleanup();
}

HWND MainWin::hFindDlg()
{
    return m_hFindDlg;
}

const wchar_t* MainWin::className(HINSTANCE hInstance)
{
    return config(hInstance).className.c_str();
}

LRESULT MainWin::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = -1;
    switch (message)
    {
    case WM_COMMAND:
        {
            UINT wmId = LOWORD(wParam);
            switch (LOWORD(wParam))
            {
            case IDM_OPEN:
                result = onOpenFile(message, wParam, lParam);
                break;
            case IDM_CLEAR:
                result = onClear(message, wParam, lParam);
                break;
            case IDM_EXIT:
                DestroyWindow(m_hWnd);
                break;
            case IDM_VIEW:
                result = onView(message, wParam, lParam);
                break;
            case IDM_MARK:
                result = onMark(message, wParam, lParam);
                break;
            case IDM_DISPLAY:
                result = onOpenDisplaySettingDlg(message, wParam, lParam);
                break;
            case IDM_CONFIG:
                result = onOpenBasicSettingDlg(message, wParam, lParam);
                break;
            case IDM_ADVSET:
                result = onOpenAdvanceSettingDlg(message, wParam, lParam);
                break;
            case IDM_KEYSET:
                result = onOpenKeySettingDlg(message, wParam, lParam);
                break;
            case IDM_TAGSET:
                result = onOpenTagSettingDlg(message, wParam, lParam);
                break;
            case IDM_ONLINE:
                result = onOpenOnlineSearchDlg(message, wParam, lParam);
                break;
            case IDM_WEB:
                result = onOpenWebDlg(message, wParam, lParam);
                break;
            case IDM_DEFAULT:
                result = onRestoreDefault(message, wParam, lParam);
                break;
            case IDM_NETWORK:
                result = onOpenNetworkSettingDlg(message, wParam, lParam);
                break;
            case IDM_ABOUT:
                result = onOpenAboutDlg(message, wParam, lParam);
                break;
            default:
                if (wmId >= IDM_OPEN_BEGIN && wmId <= IDM_OPEN_END)
                {
                    int bookIndex = wmId - IDM_OPEN_BEGIN;
                    m_session->open(bookIndex);
                    result = TRUE;
                }
                break;
            }
        }
        break;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) pauseAutoPage();
        else resumeAutoPage();
        break;
    case WM_CREATE:
        result = onCreate();
        break;
    case WM_DESTROY:
        result = onDestroy();
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hWnd, &ps);
            result = onPaint(hdc);
            EndPaint(m_hWnd, &ps);
        }
        break;
    case WM_ERASEBKGND:
        result = TRUE;
        break;
    case WM_NCHITTEST:
        result = onNcHitTest(message, wParam, lParam);
        break;
    case WM_CLOSE:
        result = onClose(message, wParam, lParam);
        break;
    case WM_SIZE:
        result = onSize(message, wParam, lParam);
        break;
    case WM_MOVE:
        result = onMove(message, wParam, lParam);
        break;
    case WM_NOTIFY:
        result = onNotify(message, wParam, lParam);
        break;
    case WM_CONTEXTMENU:
        result = onContextMenu(message, wParam, lParam);
        break;
    case WM_MENURBUTTONUP:
        result = onMenuRButtonUp(message, wParam, lParam);
        break;
    case WM_KEYDOWN:
        result = onKeyDown(message, wParam, lParam);
        break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
        result = onMouseDown(message, wParam, lParam);
        break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_NCLBUTTONUP:
    case WM_NCRBUTTONUP:
        result = onMouseUp(message, wParam, lParam);
        break;
    case WM_MOUSEWHEEL:
        result = onMouseWheel(message, wParam, lParam);
        break;
    case WM_TIMER:
        result = onTimer(message, wParam, lParam);
        break;
    case WM_DROPFILES:
        result = onDropFiles(message, wParam, lParam);
        break;
    case WM_DPICHANGED:
        result = onDpiChanged(message, wParam, lParam);
        break;
    case WM_COPYDATA:
        result = onCopyData(message, wParam, lParam);
        break;
    case WM_HOTKEY:
        result = onHotKey(message, wParam, lParam);
        break;
    case WM_SYSTRAY:
        result = onSysTray(message, wParam, lParam);
        break;
    case WM_SAVESETTING:
        result = onSaveSettingResult(message, wParam, lParam);
        break;
    case WM_NEWVERSION:
        result = onNewVersion(message, wParam, lParam);
        break;
    case WM_BOOKSTATECHANGED:
        result = onBookStateChanged(message, wParam, lParam);
        break;
    case WM_BOOKEVENT:
        result = onBookEvent(message, wParam, lParam);
        break;
    case WM_REDRAW:
        result = onRedraw(message, wParam, lParam);
        break;
    case WM_UPDATEBOOKS:
        result = onUpdateBooks(message, wParam, lParam);
        break;
    case WM_UPDATECHAPTERS:
        result = onUpdateChapters(message, wParam, lParam);
        break;
    case WM_UPDATEMARKS:
        result = onUpdateMarks(message, wParam, lParam);
        break;
    default:
        if (message == m_uFindMsgId)
            result = onSearch(message, wParam, lParam);
        break;
    }
    return result;
}

void MainWin::prevCreate()
{
    initHttps();
    m_isCreating = true;
}

void MainWin::postCreate()
{
    SetWindowPlacement(m_hWnd, &m_data.window.placement);
    m_isCreating = false;
    switchDisplayMode(m_data.window.mode);
    minToTray(m_data.window.minToTray);
    hideTaskbar(m_data.window.hideTaskbar);
    registerHotKey(true);
    initTimers();
    m_session->init();
}

const WinConfig& MainWin::config(HINSTANCE hInstance)
{
    static WinConfig cfg =
    {
        loadString(hInstance, IDC_READER),
        loadString(hInstance, IDS_APP_TITLE),
        MAKEINTRESOURCE(IDC_READER),
        MAKEINTRESOURCE(IDI_READER),
        MAKEINTRESOURCE(IDI_READER),
        Settings::instance().data().window.style,
        Settings::instance().data().window.exStyle,
        Settings::instance().data().window.placement.rcNormalPosition,
    };
    return cfg;
}

MainWin::KeyProc MainWin::keyProc(KeyId id)
{
    static const std::vector<KeyProc> KEY_PROC =
    {
        std::bind(&MainWin::onHideWindow,     this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onHideBorder,     this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onFullScreen,     this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onTopMost,        this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onOpenFile,       this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onAddMark,        this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onAutoPage,       this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onSearch,         this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onJump,           this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onEditMode,       this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onPageUp,         this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onPageDown,       this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onLineUp,         this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onLineDown,       this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onChapterUp,      this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onChapterDown,    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onFontZoomIn,     this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onFontZoomOut,    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onRefreshChapter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onRefreshContent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&MainWin::onMouseHide,      this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
    };
    return KEY_PROC[(int)id];
}

LRESULT MainWin::onCreate()
{
    m_hMenu = GetMenu(m_hWnd);
    m_hStatusBar = CreateWindow(STATUSCLASSNAME, L"Status Bar.", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hWnd, nullptr, m_hInstance, nullptr);
    m_hTreeView = CreateWindow(WC_TREEVIEW, L"Tree View", /*WS_VISIBLE | */WS_CHILD /*| WS_BORDER*/ | TVS_HASLINES | TVS_NOHSCROLL | TVS_NOTOOLTIPS | TVS_LINESATROOT, 0, 0, 0, 0, m_hWnd, nullptr, m_hInstance, nullptr);
    m_hTreeMark = CreateWindow(WC_TREEVIEW, L"Tree Mark", /*WS_VISIBLE | */WS_CHILD /*| WS_BORDER*/ | TVS_HASLINES | TVS_NOHSCROLL | TVS_NOTOOLTIPS | TVS_LINESATROOT, 0, 0, 0, 0, m_hWnd, nullptr, m_hInstance, nullptr);
    m_uFindMsgId = RegisterWindowMessage(FINDMSGSTRING);
    m_session = new BookSession(m_hWnd);
    Settings::instance().setHwnd(m_hWnd);
    updateTreeViewFont();
    return TRUE;
}

LRESULT MainWin::onDestroy()
{
    closeFindDialog();
    registerHotKey(false);
    minToTray(false);
    PostQuitMessage(0);
    return TRUE;
}

LRESULT MainWin::onPaint(HDC hdc)
{
    BYTE alpha = hdc ? 0xff : m_data.window.alpha;
    RECT rcPaint = drawTextRect();

    IRenderer::instance()->beginPaint(m_hWnd, hdc, rcPaint, m_session->styles());

    // draw background
    Image* imgBackground = loadBgImage(rcPaint.right - rcPaint.left, rcPaint.bottom - rcPaint.top, alpha);
    if (imgBackground)
    {
        IRenderer::instance()->drawImage(imgBackground, rcPaint);
    }
    else
    {
        IRenderer::instance()->fillRect(alpha << 24 | m_data.background.color, rcPaint);
    }

    if (m_session->isLoading())
    {
        RECT rc = { 0 };
        rc.left = (rcPaint.right - rcPaint.left - m_imgLoading->width()) / 2;
        rc.top = (rcPaint.bottom - rcPaint.top - m_imgLoading->height()) / 2;
        rc.right = rc.left + m_imgLoading->width();
        rc.bottom = rc.top + m_imgLoading->height();
        IRenderer::instance()->drawImage(m_imgLoading, rc);
    }
    else
    {
        m_session->drawPage(hdc, rcPaint);
    }

    IRenderer::instance()->endPaint();

    updateTitle();
    updateStatusBar();
    return TRUE;
}

LRESULT MainWin::onNcHitTest(UINT message, WPARAM wParam, LPARAM lParam)
{
    static const int BORDER_WIDTH = 8;
    static const int CAPTION_HEIGHT = 60;
    if (DisplayMode::BORDERLESS == m_data.window.mode)
    {
        int border = BORDER_WIDTH;
        DpiAwareness::instance().logicalToPhysical(border, border);

        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(m_hWnd, &pt);
        RECT rc = drawTextRect();
        if (pt.x < border && pt.y < border)
        {
            return HTTOPLEFT;
        }
        else if (pt.x > rc.right - border && pt.y < border)
        {
            return HTTOPRIGHT;
        }
        else if (pt.x < border && pt.y > rc.bottom - border)
        {
            return HTBOTTOMLEFT;
        }
        else if (pt.x > rc.right - border && pt.y > rc.bottom - border)
        {
            return HTBOTTOMRIGHT;
        }
        else if (pt.x < border)
        {
            return HTLEFT;
        }
        else if (pt.x > rc.right - border)
        {
            return HTRIGHT;
        }
        else if (pt.y < border)
        {
            return HTTOP;
        }
        else if (pt.y > rc.bottom - border)
        {
            return HTBOTTOM;
        }
        else
        {
            if (MousePageMode::CLICK_SIDES == m_data.paging.mouseMode)
            {
                rc.left = rc.right / 3;
                rc.right = rc.right / 3 * 2;
                if (PtInRect(&rc, pt))
                {
                    return HTCAPTION;
                }
            }
            else
            {
                int captionHeight = CAPTION_HEIGHT;
                DpiAwareness::instance().logicalToPhysical(captionHeight, captionHeight);
                rc.bottom = rc.bottom / 2 > captionHeight ? captionHeight : rc.bottom / 2;
                if (PtInRect(&rc, pt))
                {
                    return HTCAPTION;
                }
            }
        }
        return HTCLIENT;
    }
    return -1;
}

LRESULT MainWin::onClose(UINT message, WPARAM wParam, LPARAM lParam)
{
    stopAutoPage();
    if (m_data.window.minToTray)
    {
        wcscpy(m_nid.szInfoTitle, loadString(IDS_APP_TITLE).c_str());
        wcscpy(m_nid.szInfo, L"I'm here!");
        m_nid.uTimeout = 1000;
        Shell_NotifyIcon(NIM_MODIFY, &m_nid);
        showWindow(false);
        return TRUE;
    }
    return -1;
}

LRESULT MainWin::onSize(UINT message, WPARAM wParam, LPARAM lParam)
{
    SendMessage(m_hStatusBar, WM_SIZE, wParam, lParam);
    if (m_hEditCtrl && m_hEditCtrl->isEditing()) m_hEditCtrl->updateLayout();
    if (SIZE_RESTORED == wParam)
    {
        resumeAutoPage();
        saveWindowPlacement();
    }
    else if (SIZE_MAXIMIZED == wParam)
    {
        resumeAutoPage();
    }
    else if (SIZE_MINIMIZED == wParam)
    {
        pauseAutoPage();
    }
    if (DisplayMode::BORDERLESS == m_data.window.mode && !m_isSwitching)
    {
        redrawWindow();
    }
    return -1;
}

LRESULT MainWin::onMove(UINT message, WPARAM wParam, LPARAM lParam)
{
    saveWindowPlacement();
    return -1;
}

LRESULT MainWin::onNotify(UINT message, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR pnmh = (LPNMHDR)lParam;
    switch (pnmh->code)
    {
    case TVN_SELCHANGED:
        if (pnmh->hwndFrom == m_hTreeView)
        {
            if (IsWindowVisible(m_hTreeView))
            {
                HTREEITEM selected = TreeView_GetSelection(m_hTreeView);
                if (selected)
                {
                    TVITEM item = { 0 };
                    item.mask = TVIF_PARAM;
                    item.hItem = selected;
                    TreeView_GetItem(m_hTreeView, &item);
                    m_session->seekChapter((int)item.lParam);
                    PostMessage(m_hWnd, WM_COMMAND, IDM_VIEW, NULL);
                }
            }
        }
        break;
    case NM_RCLICK:
        if (pnmh->hwndFrom == m_hTreeView)
        {
            HTREEITEM hItem = TreeView_GetNextItem(m_hTreeView, 0, TVGN_DROPHILITE);
            if (hItem)
                TreeView_SelectItem(m_hTreeView, hItem);
        }
        if (pnmh->hwndFrom == m_hTreeMark)
        {
            HTREEITEM hItem = TreeView_GetNextItem(m_hTreeMark, 0, TVGN_DROPHILITE);
            if (hItem)
                TreeView_SelectItem(m_hTreeMark, hItem);
        }
        break;
    case NM_DBLCLK:
        if (pnmh->hwndFrom == m_hTreeMark)
        {
            if (IsWindowVisible(m_hTreeMark))
            {
                HTREEITEM selected = TreeView_GetSelection(m_hTreeMark);
                if (selected)
                {
                    TVITEM item = { 0 };
                    item.mask = TVIF_PARAM;
                    item.hItem = selected;
                    TreeView_GetItem(m_hTreeMark, &item);
                    m_session->seek((int)item.lParam);
                    PostMessage(m_hWnd, WM_COMMAND, IDM_MARK, NULL);
                }
            }
        }
        break;
    }
    return -1;
}

LRESULT MainWin::onContextMenu(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (wParam == (WPARAM)m_hTreeMark)
    {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        HTREEITEM selected = TreeView_GetSelection(m_hTreeMark);
        if (selected)
        {
            HMENU hMenu = CreatePopupMenu();
            if (hMenu)
            {
                AppendMenu(hMenu, MF_STRING, IDM_DELETE, loadString(IDS_DELETE).c_str());
                AppendMenu(hMenu, MF_STRING, IDM_CLEAR, loadString(IDS_CLEAR).c_str());
                int ret = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hTreeMark, NULL);
                DestroyMenu(hMenu);
                if (IDM_DELETE == ret)
                {
                    if (IDYES == messageBox(IDS_DELETE_CONFIRM, IDS_TIPS, MB_ICONQUESTION | MB_YESNO))
                    {
                        TVITEM item;
                        item.mask = TVIF_PARAM;
                        item.hItem = selected;
                        TreeView_GetItem(m_hTreeMark, &item);
                        TreeView_DeleteItem(m_hTreeMark, selected);
                        m_session->remMark((int)item.lParam);
                    }
                }
                else if (IDM_CLEAR == ret)
                {
                    if (IDYES == messageBox(IDS_CLEAR_CONFIRM, IDS_TIPS, MB_ICONQUESTION | MB_YESNO))
                    {
                        m_session->clrMark();
                    }
                }
            }
        }
    }
    return -1;
}

LRESULT MainWin::onMenuRButtonUp(UINT message, WPARAM wParam, LPARAM lParam)
{
    UINT wmId = GetMenuItemID((HMENU)lParam, (int)wParam);
    if (wmId >= IDM_OPEN_BEGIN && wmId <= IDM_OPEN_END)
    {
        int bookIndex = wmId - IDM_OPEN_BEGIN;
        if (bookIndex >= 0 && bookIndex < (int)m_data.books.size())
        {
            auto& book = m_data.books[bookIndex];
            if (IDYES == messageBoxFmt(IDS_DELETE_FILE_CFM, IDS_TIPS, MB_ICONQUESTION | MB_YESNO, book.fileName.c_str()))
            {
                m_session->remove(book.fileName);
            }
        }
    }
    return -1;
}

LRESULT MainWin::onKeyDown(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (VK_ESCAPE == wParam)
    {
        if (DisplayMode::FULLSCREEN == m_data.window.mode)
        {
            onFullScreen(message, wParam, lParam);
            return -1;
        }
        else
        {
            if (m_data.keyset.enableEscHide)
            {
                showWindow(false);
                return -1;
            }
        }
    }

    auto& keys = m_data.keyset.keys;
    DWORD value = toModifiers(wParam);
    for (auto& k : keys)
    {
        if (k.enable && k.value == value)
        {
            return keyProc(k.id)(message, wParam, lParam);
        }
    }
    return -1;
}

LRESULT MainWin::onMouseDown(UINT message, WPARAM wParam, LPARAM lParam)
{
    auto hideTreeView = [this](HWND tv, int pos)
        {
            if (IsWindowVisible(tv))
            {
                ShowWindow(tv, SW_HIDE);
                resumeAutoPage();
                RECT rc;
                POINT pt;
                GetCursorPos(&pt);
                GetMenuItemRect(m_hWnd, GetMenu(m_hWnd), pos, &rc);
                if (PtInRect(&rc, pt))
                {
                    return TRUE;
                }
            }
            return FALSE;
        };

    auto mouseLRHide = [&]()
        {
            if ((wParam & MK_LBUTTON) && (wParam & MK_RBUTTON) && m_data.keyset.enableMouseLRHide)
            {
                showWindow(false);
                return TRUE;
            }
            return FALSE;
        };

    switch (message)
    {
    case WM_LBUTTONDOWN:
        if (mouseLRHide()) return TRUE;
        if (MousePageMode::CLICK_ANYWHERE == m_data.paging.mouseMode)
        {
            return onPageDown(message, wParam, lParam);
        }
        else if (MousePageMode::CLICK_SIDES == m_data.paging.mouseMode)
        {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(m_hWnd, &pt);
            RECT rc = drawTextRect();
            rc.left = rc.right / 3 * 2;
            if (PtInRect(&rc, pt))
            {
                return onPageDown(message, wParam, lParam);
            }
            else
            {
                rc.left = 0;
                rc.right = rc.right / 3;
                if (PtInRect(&rc, pt))
                {
                    return onPageUp(message, wParam, lParam);
                }
            }
        }
        break;
    case WM_RBUTTONDOWN:
        if (mouseLRHide()) return TRUE;
        if (MousePageMode::CLICK_ANYWHERE == m_data.paging.mouseMode)
        {
            return onPageUp(message, wParam, lParam);
        }
        break;
    case WM_MBUTTONDOWN:
        showWindow(false);
        return TRUE;
    case WM_NCLBUTTONDOWN:
        if (mouseLRHide()) return TRUE;
        if (hideTreeView(m_hTreeView, 1)) return TRUE;
        if (hideTreeView(m_hTreeMark, 2)) return TRUE;
        break;
    case WM_NCRBUTTONDOWN:
        if (mouseLRHide()) return TRUE;
        if (hideTreeView(m_hTreeView, 1)) return TRUE;
        if (hideTreeView(m_hTreeMark, 2)) return TRUE;
        break;
    default:
        break;
    }
    return -1;
}

LRESULT MainWin::onMouseUp(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_NCLBUTTONUP:
    case WM_NCRBUTTONUP:
        break;
    default:
        break;
    }
    return -1;
}

LRESULT MainWin::onMouseWheel(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (IsWindowVisible(m_hTreeView))
    {
        SetFocus(m_hTreeView);
        return TRUE;
    }
    if (IsWindowVisible(m_hTreeMark))
    {
        SetFocus(m_hTreeMark);
        return TRUE;
    }

    const WORD keys    = GET_KEYSTATE_WPARAM(wParam);
    const bool isCtrl  = (keys & MK_CONTROL) != 0;
    const bool isShift = (keys & MK_SHIFT) != 0;
    const bool isAlt   = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

    auto setAlpha = [this](BYTE alpha) {
        if (m_data.window.alpha == alpha) return;
        m_data.window.alpha = alpha;
        if (DisplayMode::NORMAL == m_data.window.mode)
            SetLayeredWindowAttributes(m_hWnd, 0, m_data.window.alpha < MIN_LAYERED_ALPHA ? MIN_LAYERED_ALPHA : m_data.window.alpha, LWA_ALPHA);
        else
            redrawWindow();
        Settings::instance().save(300);
    };

    auto setFontAlpha = [this](BYTE alpha) {
        if (m_data.window.textAlpha == alpha) return;
        m_data.window.textAlpha = alpha;
        m_session->updateStyles();
        unloadBgImage();
        Settings::instance().save(300);
    };

    if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
    {
        if (isCtrl)
        {
            // set background alpha
            if (isShift)
            {
                setAlpha(MAX_ALPHA);
            }
            else
            {
                if (m_data.window.alpha < MAX_ALPHA - UNIT_STEP)
                    setAlpha(m_data.window.alpha + UNIT_STEP);
                else
                    setAlpha(MAX_ALPHA);
            }
        }
        else if (isAlt)
        {
            // set font alpha
            if (isShift)
            {
                setFontAlpha(MAX_TEXT_ALPHA);
            }
            else
            {
                if (m_data.window.textAlpha < MAX_TEXT_ALPHA - UNIT_STEP)
                    setFontAlpha(m_data.window.textAlpha + UNIT_STEP);
                else
                    setFontAlpha(MAX_TEXT_ALPHA);
            }
        }
        else
        {
            onLineUp(message, wParam, lParam);
        }
        return TRUE;
    }
    else
    {
        if (isCtrl)
        {
            // set background alpha
            if (isShift)
            {
                setAlpha(MIN_ALPHA);
            }
            else
            {
                if (m_data.window.alpha > MIN_ALPHA + UNIT_STEP)
                    setAlpha(m_data.window.alpha - UNIT_STEP);
                else
                    setAlpha(MIN_ALPHA);
            }
        }
        else if (isAlt)
        {
            // set font alpha
            if (isShift)
            {
                setFontAlpha(MIN_TEXT_ALPHA);
            }
            else
            {
                if (m_data.window.textAlpha > MIN_TEXT_ALPHA + UNIT_STEP)
                    setFontAlpha(m_data.window.textAlpha - UNIT_STEP);
                else
                    setFontAlpha(MIN_TEXT_ALPHA);
            }
        }
        else
        {
            onLineDown(message, wParam, lParam);
        }
        return TRUE;
    }
    return -1;
}

LRESULT MainWin::onTimer(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
#ifdef ENABLE_NETWORK
    case IDT_UPGRADE:
        UpgradeDlg::checkVersion(m_hWnd);
        break;
    case IDT_CHECKBOOK:
        m_session->checkBookUpdate();
        break;
#endif
    case IDT_PLAYLOADING:
        m_imgLoading->activeFrame((m_imgLoading->frameIndex() + 1) % m_imgLoading->frameCount());
        playLoading();
        break;
    case IDT_SAVESETTING:
        KillTimer(m_hWnd, IDT_SAVESETTING);
        Settings::instance().save();
        break;
    case IDT_AUTOPAGING:
        if (AutoPageMode::PAGE_FLIP == m_data.paging.autoMode)
        {
            onPageDown(message, wParam, lParam);
        }
        else if (AutoPageMode::LINE_SCROLL == m_data.paging.autoMode)
        {
            onLineDown(message, wParam, lParam);
        }
        if (!m_session->isValid() || m_session->isEnd())
        {
            stopAutoPage();
        }
        else
        {
            applyAutoPageTimer();
        }
        break;
    case IDT_BOOKSYNCPROGRESS:
    case IDT_BOOKSAVEFILE:
    case IDT_BOOKIDLEFETCH:
        m_session->onTimer(wParam, lParam);
        break;
    default:
        break;
    }
    return TRUE;
}

LRESULT MainWin::onDropFiles(UINT message, WPARAM wParam, LPARAM lParam)
{
    HDROP hDropInfo = (HDROP)wParam;
    UINT  nFileCount = DragQueryFileW(hDropInfo, (UINT)-1, NULL, 0);
    wchar_t fileName[MAX_PATH] = { 0 };

    for (UINT i = 0; i < nFileCount; i++)
    {
        DragQueryFileW(hDropInfo, i, fileName, sizeof(fileName));
        if (GetFileAttributesW(fileName) & FILE_ATTRIBUTE_DIRECTORY)
        {
            continue;
        }
        m_session->open(fileName);
        break;
    }
    DragFinish(hDropInfo);
    return TRUE;
}

LRESULT MainWin::onDpiChanged(UINT message, WPARAM wParam, LPARAM lParam)
{
    UINT newDpiX = LOWORD(wParam);
    UINT newDpiY = HIWORD(wParam);
    UINT oldDpi = DpiAwareness::instance().dpi();
    LPRECT pRect = (LPRECT)lParam;

    if (oldDpi != newDpiX && oldDpi > 0)
    {
        Settings::instance().toLogical(m_data);
        DpiAwareness::instance().setDpi(newDpiX, newDpiY);
        Settings::instance().toPhysical(m_data);
        Settings::instance().save();
        m_session->updateStyles();
        unloadBgImage();
        updateTreeViewFont();
    }
    if (pRect)
    {
        SetWindowPos(m_hWnd, HWND_TOP, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return TRUE;
}

LRESULT MainWin::onCopyData(UINT message, WPARAM wParam, LPARAM lParam)
{
    COPYDATASTRUCT *pCopyData = (COPYDATASTRUCT*)lParam;
    wchar_t fileName[MAX_PATH];

    switch (pCopyData->dwData)
    {
    case 0: // extern open file
        GetFullPathName((wchar_t*)pCopyData->lpData, MAX_PATH, fileName, NULL);
        m_session->open(fileName);
        break;
    }
    return TRUE;
}

LRESULT MainWin::onHotKey(UINT message, WPARAM wParam, LPARAM lParam)
{
    for (auto& k : m_data.keyset.keys)
    {
        if (KeyType::HOTKEY == k.type && (int)k.id == wParam)
        {
            return keyProc(k.id)(message, wParam, lParam);
        }
    }
    return -1;
}

LRESULT MainWin::onSysTray(UINT message, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    HMENU hMenu;
    int ret;
    switch (lParam)
    {
    case WM_LBUTTONUP:
        if (!IsWindowVisible(m_hWnd))
            ShowWindow(m_hWnd, SW_SHOW);
        if (IsIconic(m_hWnd))
            ShowWindow(m_hWnd, SW_RESTORE);
        SetForegroundWindow(m_hWnd);
        break;
    case WM_RBUTTONDOWN:
    case WM_CONTEXTMENU:
        GetCursorPos(&pt);
        hMenu = CreatePopupMenu();
        if (hMenu)
        {
            InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, IDM_ST_OPEN, loadString(IDS_OPEN).c_str());
            InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, IDM_ST_EXIT, loadString(IDS_EXIT).c_str());
            SetForegroundWindow(m_hWnd);
            ret = TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
            DestroyMenu(hMenu);
            if (IDM_ST_OPEN == ret)
            {
                showWindow(true);
            }
            else if (IDM_ST_EXIT == ret)
            {
                DestroyWindow(m_hWnd);
            }
        }
        break;
    default:
        break;
    }
    return TRUE;
}

LRESULT MainWin::onOpenDisplaySettingDlg(UINT message, WPARAM wParam, LPARAM lParam)
{
    DisplaySettingDlg dlg(m_hInstance, m_hWnd);
    LRESULT result = dlg.exec();
    if (dlg.isChanged())
    {
        SetLayeredWindowAttributes(m_hWnd, 0, m_data.window.alpha < MIN_LAYERED_ALPHA ? MIN_LAYERED_ALPHA : m_data.window.alpha, LWA_ALPHA);
        m_session->updateStyles();
        unloadBgImage();
        updateTreeViewFont();
    }
    return result;
}

LRESULT MainWin::onOpenBasicSettingDlg(UINT message, WPARAM wParam, LPARAM lParam)
{
    BasicSettingDlg dlg(m_hInstance, m_hWnd);
    LRESULT result = dlg.exec();
    if (dlg.isMinToTrayChanged())
    {
        minToTray(m_data.window.minToTray);
    }
    if (dlg.isHideTaskbarChanged())
    {
        hideTaskbar(m_data.window.hideTaskbar);
    }
    return result;
}

LRESULT MainWin::onOpenAdvanceSettingDlg(UINT message, WPARAM wParam, LPARAM lParam)
{
    AdvanceSettingDlg dlg(m_hInstance, m_hWnd);
    LRESULT result = dlg.exec();
    if (dlg.isChanged() && m_session->isValid() && BookType::TEXT == m_session->book()->type())
    {
        m_session->reopen();
    }
    return result;
}

LRESULT MainWin::onOpenKeySettingDlg(UINT message, WPARAM wParam, LPARAM lParam)
{
    KeySettingDlg dlg(m_hInstance, m_hWnd);
    auto ret = dlg.exec();

    if (m_hMouseHook)
    {
        for (auto &i : m_data.keyset.keys)
        {
            if (i.id == KeyId::MOUSE_HIDE && (!i.enable))
            {
                UnhookWindowsHookEx(m_hMouseHook);
                m_hMouseHook = NULL;
                break;
            }
        }
    }
    return ret;
}

LRESULT MainWin::onOpenTagSettingDlg(UINT message, WPARAM wParam, LPARAM lParam)
{
#if ENABLE_TAG
    TagMgrDlg dlg(m_hInstance, m_hWnd);
    auto ret = dlg.exec();
    m_session->updateStyles();
    unloadBgImage();
    return ret;
#else
    return TRUE;
#endif
}

LRESULT MainWin::onOpenOnlineSearchDlg(UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef ENABLE_NETWORK
    OnlineSearchDlg dlg(m_hInstance, m_hWnd);
    LRESULT result = dlg.exec();
    std::wstring fileName = dlg.openFileName();
    if (!fileName.empty())
    {
        m_session->open(fileName, true);
    }
    return result;
#else
    return -1;
#endif
}

LRESULT MainWin::onOpenWebDlg(UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef ENABLE_NETWORK
    WebDlg dlg(m_hInstance, m_hWnd);
    LRESULT result = dlg.exec();
    std::wstring fileName = dlg.openFileName();
    if (!fileName.empty())
    {
        m_session->open(fileName, true);
    }
    return result;
#else
    return -1;
#endif
}

LRESULT MainWin::onOpenNetworkSettingDlg(UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef ENABLE_NETWORK
    NetworkSettingDlg dlg(m_hInstance, m_hWnd);
    LRESULT result = dlg.exec();
    if (dlg.isChanged())
    {
        setHttpsParam();
    }
    return result;
#else
    return -1;
#endif
}

LRESULT MainWin::onOpenAboutDlg(UINT message, WPARAM wParam, LPARAM lParam)
{
    AboutDlg dlg(m_hInstance, m_hWnd);
    return dlg.exec();
}

LRESULT MainWin::onRestoreDefault(UINT message, WPARAM wParam, LPARAM lParam)
{
    auto oldData = m_data;
    if (IDYES != messageBox(IDS_RESTORE_DEFAULT, IDS_TIPS, MB_ICONQUESTION | MB_YESNO))
    {
        return FALSE;
    }

    stopAutoPage();
    registerHotKey(false);
    Settings::instance().restoreDefault();

    if (oldData.chapterRule.rule != m_data.chapterRule.rule)
    {
        m_session->reopen();
    }
    m_session->updateStyles();
    unloadBgImage();

    switchDisplayMode(m_data.window.mode);
    minToTray(m_data.window.minToTray);
    hideTaskbar(m_data.window.hideTaskbar);
    updateTreeViewFont();
    registerHotKey(true);
    return TRUE;
}

LRESULT MainWin::onClear(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (m_data.books.empty())
    {
        return TRUE;
    }
    if (IDYES != messageBox(IDS_CLEAR_ALL_FILES, IDS_WARN, MB_ICONINFORMATION | MB_YESNO))
    {
        return TRUE;
    }
    m_session->clear();
    return TRUE;
}

LRESULT MainWin::onView(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (IsWindowVisible(m_hTreeView))
    {
        ShowWindow(m_hTreeView, SW_HIDE);
        resumeAutoPage();
    }
    else
    {
        if (m_session->isValid())
        {
            if (m_session->book()->chapterCount() == 0) return TRUE;
            int index = m_session->book()->curChapterIndex();
            if (index == -1) return TRUE;

            HTREEITEM hti = TreeView_GetRoot(m_hTreeView);
            do
            {
                TVITEM tvi = { 0 };
                tvi.mask = TVIF_PARAM;
                tvi.hItem = hti;
                TreeView_GetItem(m_hTreeView, &tvi);
                if (tvi.lParam == index)
                {
                    TreeView_EnsureVisible(m_hTreeView, hti);
                    TreeView_SelectItem(m_hTreeView, hti);
                    break;
                }
                hti = TreeView_GetNextSibling(m_hTreeView, hti);
            } while (hti);

            RECT rc = drawTextRect();
            SetWindowPos(m_hTreeView, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);
            SetFocus(m_hTreeView);
            pauseAutoPage();
        }
    }
    return TRUE;
}

LRESULT MainWin::onMark(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (IsWindowVisible(m_hTreeMark))
    {
        ShowWindow(m_hTreeMark, SW_HIDE);
        resumeAutoPage();
    }
    else
    {
        if (m_session->isValid())
        {
            if (m_session->marks().empty())
            {
                return TRUE;
            }
            RECT rc = drawTextRect();
            SetWindowPos(m_hTreeMark, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);
            SetFocus(m_hTreeMark);
            pauseAutoPage();
        }
    }
    return TRUE;
}

LRESULT MainWin::onSaveSettingResult(UINT message, WPARAM wParam, LPARAM lParam)
{
    Utils::lastErrorMessage((DWORD)wParam);
    messageBoxFmt(IDS_SAVESETTING_FAIL, IDS_ERROR, MB_OK | MB_ICONERROR, Utils::lastErrorMessage((DWORD)wParam).c_str());
    return TRUE;
}

LRESULT MainWin::onNewVersion(UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef ENABLE_NETWORK
    UpgradeDlg dlg(m_hInstance, m_hWnd);
    dlg.setVersion((VersionInfo*)lParam);
    return dlg.exec();
#else
    return -1;
#endif
}

LRESULT MainWin::onBookStateChanged(UINT message, WPARAM wParam, LPARAM lParam)
{
    BookState state = (BookState)lParam;
    switch (state)
    {
    case BookState::CLOSED:
        stopLoading();
        stopAutoPage();
        break;
    case BookState::LOADING:
        playLoading();
        pauseAutoPage();
        break;
    case BookState::OPENED:
        stopLoading();
        resumeAutoPage();
        break;
    case BookState::UNREADY:
    case BookState::FAILED:
    {
        stopLoading();
        stopAutoPage();
        BookErrno err = m_session->lastError();
        bool retryable = (BookState::FAILED == state) && (BookErrno::REQUEST_FAIL == err || BookErrno::HTTP_STATUS_ERROR == err || BookErrno::XPATH_FAIL == err || BookErrno::JSON_FAIL == err || BookErrno::JSON_NODATA == err);
        bool isWeb = m_session->book() && m_session->book()->type() == BookType::WEB;
        UINT uType = retryable ? MB_RETRYCANCEL : MB_OK;
        UINT uIcon = (BookState::FAILED == state) ? MB_ICONERROR : MB_ICONWARNING;
        int ret = 0;
        switch (err)
        {
        case BookErrno::FAIL:
            ret = messageBox(IDS_FILE_OPEN_FAIL, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::NOT_EXIST:
            ret = messageBox(IDS_FILE_NOT_EXIST, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::UNKNOWN_FORMAT:
            ret = messageBox(IDS_UNKNOWN_FORMAT, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::EMPTY:
            ret = messageBox(IDS_FILE_IS_EMPTY, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::TOO_LARGE:
            ret = messageBox(IDS_FILE_TOO_LARGE, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::OPEN_R_FAIL:
            ret = messageBox(IDS_FILE_OPEN_FAIL, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::OPEN_W_FAIL:
            ret = messageBox(IDS_OPEN_FILE_FAILED, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::READ_FAIL:
            ret = messageBox(IDS_FILE_READ_FAIL, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::WRITE_FAIL:
            ret = messageBox(IDS_WRITE_FILE_FAILED, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::NOT_SUPPORT:
            ret = messageBox(IDS_NOT_SUPPORT, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::BOOKSOURCE_NOT_FOUND:
            ret = messageBox(IDS_NOT_FOUND_BS_FOR_OPEN, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::PARSE_FAIL:
            ret = messageBox(IDS_PARSE_FILE_FAIL, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::REQUEST_FAIL:
            ret = messageBox(isWeb ? IDS_WEB_REQUEST_FAIL : IDS_NET_REQUEST_FAIL, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::HTTP_STATUS_ERROR:
            ret = messageBox(isWeb ? IDS_WEB_STATUS_ERROR : IDS_HTTP_STATUS_ERROR, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::XPATH_FAIL:
            ret = messageBox(IDS_HTML_XPATH_PARSE_FAIL, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::JSON_FAIL:
            ret = messageBox(IDS_JSON_PARSE_FAIL, IDS_ERROR, uType | uIcon);
            break;
        case BookErrno::JSON_NODATA:
            ret = messageBox(IDS_JSON_PARSE_NODATA, IDS_ERROR, uType | uIcon);
            break;
        default:
            ret = messageBox(IDS_FILE_OPEN_FAIL, IDS_ERROR, uType | uIcon);
            break;
        }

        if (ret == IDRETRY)
        {
            m_session->reopen();
        }
        break;
    }
    default:
        break;
    }
    return TRUE;
}

LRESULT MainWin::onBookEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
    m_session->onEvent(wParam, lParam);
    return TRUE;
}

LRESULT MainWin::onRedraw(UINT message, WPARAM wParam, LPARAM lParam)
{
    redrawWindow();
    return TRUE;
}

LRESULT MainWin::onUpdateBooks(UINT message, WPARAM wParam, LPARAM lParam)
{
#if !ENABLE_TAG
    deleteMenu(m_hMenu, IDM_TAGSET, FALSE);
#endif

#ifndef ENABLE_NETWORK
    deleteMenu(m_hMenu, IDM_NETWORK, TRUE);
    deleteMenu(m_hMenu, IDM_ONLINE, TRUE);
    deleteMenu(m_hMenu, IDM_WEB, TRUE);
#endif

    HMENU hFile = GetSubMenu(m_hMenu, 0);
    if (hFile)
    {
        DeleteMenu(m_hMenu, 0, MF_BYPOSITION);
        hFile = NULL;
    }

    int bookMenuId = IDM_OPEN_BEGIN;
    hFile = CreateMenu();
    AppendMenu(hFile, MF_STRING, IDM_OPEN, loadString(IDS_MENU_OPEN).c_str());
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);

    for (auto& book : m_data.books)
    {
        AppendMenu(hFile, MF_STRING, (UINT_PTR)bookMenuId, book.fileName.c_str());
#ifdef ENABLE_NETWORK
        if (Utils::fileExt(book.fileName) == L".ol" && book.hasNew)
        {
            if (!m_hNewBitmap)
            {
                Image* image = IRenderer::instance()->loadImageFromResource(m_hInstance, IDB_PNG_NEW, L"PNG");
                m_hNewBitmap = image->getHBITMAP();
                delete image;
            }
            SetMenuItemBitmaps(hFile, bookMenuId, MF_BITMAP, m_hNewBitmap, m_hNewBitmap);
        }
#endif
        bookMenuId++;
    }
    if (!m_data.books.empty())
    {
        AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    }
    AppendMenu(hFile, MF_STRING, IDM_CLEAR, loadString(IDS_MENU_CLEAR).c_str());
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFile, MF_STRING, IDM_EXIT, loadString(IDS_MENU_EXIT).c_str());
    InsertMenu(m_hMenu, 0, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hFile, loadString(IDS_MENU_FILE).c_str());

    if (GetMenu(m_hWnd))
    {
        DrawMenuBar(m_hWnd);
    }
    return TRUE;
}

LRESULT MainWin::onUpdateChapters(UINT message, WPARAM wParam, LPARAM lParam)
{
    TreeView_DeleteAllItems(m_hTreeView);

    if (m_session->isValid())
    {
        TVITEM tvi = { 0 };
        TVINSERTSTRUCT tvins = { 0 };
        HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;

        tvi.mask = TVIF_TEXT /*| TVIF_IMAGE | TVIF_SELECTEDIMAGE */ | TVIF_PARAM;
        for (int i = 0; i < m_session->book()->chapterCount(); i++)
        {
            tvi.pszText = (LPWSTR)m_session->book()->chapter(i)->title.c_str();
            tvi.cchTextMax = (int)m_session->book()->chapter(i)->title.size();
            tvi.lParam = (LPARAM)i;
            tvins.item = tvi;
            tvins.hInsertAfter = hPrev;
            tvins.hParent = TVI_ROOT;
            hPrev = (HTREEITEM)SendMessage(m_hTreeView, TVM_INSERTITEM, 0, (LPARAM)&tvins);
        }
    }
    return TRUE;
}

LRESULT MainWin::onUpdateMarks(UINT message, WPARAM wParam, LPARAM lParam)
{
    TreeView_DeleteAllItems(m_hTreeMark);

    if (m_session->isValid())
    {
        auto& marks = m_session->marks();
        TVITEM tvi = { 0 };
        TVINSERTSTRUCT tvins = { 0 };
        HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
        const int MAX_MARK_TEXT = 255;
        int bookLen = m_session->book()->length();
        wchar_t buffer[MAX_MARK_TEXT + 1] = { 0 };

        tvi.mask = TVIF_TEXT /*| TVIF_IMAGE | TVIF_SELECTEDIMAGE */ | TVIF_PARAM;
        for (int i = 0; i < (int)marks.size(); i++)
        {
            int len = marks[i] + MAX_MARK_TEXT > bookLen ? bookLen - marks[i] : MAX_MARK_TEXT;
            memcpy(buffer, m_session->book()->text() + marks[i], len * sizeof(wchar_t));

            tvi.pszText = buffer;
            tvi.cchTextMax = len;
            tvi.lParam = (LPARAM)i;
            tvins.item = tvi;
            tvins.hInsertAfter = hPrev;
            tvins.hParent = TVI_ROOT;
            hPrev = (HTREEITEM)SendMessage(m_hTreeMark, TVM_INSERTITEM, 0, (LPARAM)&tvins);
        }
    }
    return TRUE;
}

LRESULT MainWin::onHideWindow(UINT message, WPARAM wParam, LPARAM lParam)
{
    showWindow(!IsWindowVisible(m_hWnd));
    return TRUE;
}

LRESULT MainWin::onHideBorder(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (DisplayMode::FULLSCREEN == m_data.window.mode)
    {
        return TRUE;
    }
    else if (DisplayMode::NORMAL == m_data.window.mode)
    {
        switchDisplayMode(DisplayMode::BORDERLESS);
    }
    else if (DisplayMode::BORDERLESS == m_data.window.mode)
    {
        switchDisplayMode(DisplayMode::NORMAL);
    }
    return TRUE;
}

LRESULT MainWin::onFullScreen(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (DisplayMode::FULLSCREEN == m_data.window.mode)
    {
        if (WS_CAPTION & m_data.window.style)
        {
            switchDisplayMode(DisplayMode::NORMAL);
        }
        else
        {
            switchDisplayMode(DisplayMode::BORDERLESS);
        }
    }
    else
    {
        switchDisplayMode(DisplayMode::FULLSCREEN);
    }
    return TRUE;
}

LRESULT MainWin::onTopMost(UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD exStyle = (DWORD)GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
    bool isTopMost = (exStyle & WS_EX_TOPMOST) != 0;

    SetWindowPos(m_hWnd, isTopMost ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);

    if (isTopMost) exStyle &= ~WS_EX_TOPMOST;
    else exStyle |= WS_EX_TOPMOST;

    m_data.window.exStyle = exStyle;
    Settings::instance().save();
    return TRUE;
}

LRESULT MainWin::onOpenFile(UINT message, WPARAM wParam, LPARAM lParam)
{
    wchar_t szFileName[MAX_PATH] = { 0 };
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
#ifdef ENABLE_NETWORK
    ofn.lpstrFilter = L"Files (*.txt;*.epub;*.mobi;*.ol;*.web)\0*.txt;*.epub;*.mobi;*.ol;*.web\0\0";
#else
    ofn.lpstrFilter = L"Files (*.txt;*.epub;*.mobi)\0*.txt;*.epub;*.mobi\0\0";
#endif
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName) / sizeof(*szFileName);
    ofn.nFilterIndex = 0;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    BOOL bSel = GetOpenFileNameW(&ofn);
    if (!bSel)
    {
        return TRUE;
    }
    m_session->open(szFileName);
    return TRUE;
}

LRESULT MainWin::onAddMark(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (m_session->isValid())
    {
        int ret = messageBox(IDS_ADD_BOOKMARK_TIPS, IDS_BOOKMARK, MB_ICONQUESTION | MB_YESNO);
        if (ret == IDYES)
        {
            m_session->addMark(m_session->book()->index());
        }
    }
    return 0;
}

LRESULT MainWin::onAutoPage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (m_session->isValid())
    {
        if (m_autoPageEnabled)
        {
            stopAutoPage();
        }
        else
        {
            if (!m_hEditCtrl || !m_hEditCtrl->isEditing()) startAutoPage();
        }
    }
    return TRUE;
}

LRESULT MainWin::onSearch(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == m_uFindMsgId) // do search
    {
        FINDREPLACE* fr = (FINDREPLACE*)lParam;
        if (!fr || !fr->lpstrFindWhat) return TRUE;

        if (fr->Flags & FR_DIALOGTERM)
        {
            EnableWindow(m_hWnd, TRUE);
            SetForegroundWindow(m_hWnd);
        }
        else if (fr->Flags & FR_FINDNEXT)
        {
            m_session->seek(fr->Flags & FR_DOWN, fr->lpstrFindWhat);
        }
    }
    else // WM_KEYDOWN
    {
        if (m_session->isValid()) openFindDialog();
    }
    return TRUE;
}

LRESULT MainWin::onJump(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (m_session->isValid())
    {
        JumpProgressDlg dlg(m_hInstance, m_hWnd);
        dlg.setProgress(m_session->progress());
        if (dlg.exec() == IDOK)
        {
            m_session->seek(dlg.progress());
        }
    }
    return TRUE;
}

LRESULT MainWin::onEditMode(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!m_hEditCtrl)
    {
        m_hEditCtrl = new EditController(this);
        m_hEditCtrl->create();
    }
    if (!m_hEditCtrl) return TRUE;
    if (m_autoPageEnabled) stopAutoPage();

    if (!m_hEditCtrl->isEditing()) m_hEditCtrl->beginEdit();
    else m_hEditCtrl->endEdit();
    return TRUE;
}

LRESULT MainWin::onPageUp(UINT message, WPARAM wParam, LPARAM lParam)
{
    m_session->prevPage();
    return TRUE;
}

LRESULT MainWin::onPageDown(UINT message, WPARAM wParam, LPARAM lParam)
{
    m_session->nextPage();
    return TRUE;
}

LRESULT MainWin::onLineUp(UINT message, WPARAM wParam, LPARAM lParam)
{
    m_session->prevLine();
    return TRUE;
}

LRESULT MainWin::onLineDown(UINT message, WPARAM wParam, LPARAM lParam)
{
    m_session->nextLine();
    return TRUE;
}

LRESULT MainWin::onChapterUp(UINT message, WPARAM wParam, LPARAM lParam)
{
    m_session->prevChapter();
    return TRUE;
}

LRESULT MainWin::onChapterDown(UINT message, WPARAM wParam, LPARAM lParam)
{
    m_session->nextChapter();
    return TRUE;
}

LRESULT MainWin::onFontZoomIn(UINT message, WPARAM wParam, LPARAM lParam)
{
    const int ZOOM_MAX = 72;
    if (m_session->isValid())
    {
        int pointSize = abs(MulDiv(m_data.styles.textFont.lfHeight, 72, DpiAwareness::instance().dpi()));
        if (pointSize < ZOOM_MAX)
        {
            m_data.styles.textFont.lfHeight = -MulDiv(pointSize + 1, DpiAwareness::instance().dpi(), 72);
            m_session->updateStyles();
            unloadBgImage();
            updateTreeViewFont();
        }
    }
    return TRUE;
}

LRESULT MainWin::onFontZoomOut(UINT message, WPARAM wParam, LPARAM lParam)
{
    const int ZOOM_MIN = 1;
    if (m_session->isValid())
    {
        int pointSize = abs(MulDiv(m_data.styles.textFont.lfHeight, 72, DpiAwareness::instance().dpi()));
        if (pointSize > ZOOM_MIN)
        {
            m_data.styles.textFont.lfHeight = -MulDiv(pointSize - 1, DpiAwareness::instance().dpi(), 72);
            m_session->updateStyles();
            unloadBgImage();
            updateTreeViewFont();
        }
    }
    return TRUE;
}

LRESULT MainWin::onRefreshChapter(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!m_session->isValid()) return -1;

    auto* book = m_session->book();
    if (!book || !book->isRefreshSupport()) return -1;

    if (IDYES == messageBox(IDS_REFRESH_TIP, IDS_TIPS, MB_ICONQUESTION | MB_YESNO))
    {
        m_session->refreshChapter();
    }
    return -1;
}

LRESULT MainWin::onRefreshContent(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!m_session->isValid()) return -1;

    auto* book = m_session->book();
    if (!book || !book->isRefreshSupport()) return -1;

    if (IDYES == messageBox(IDS_REFRESH_CONTENT_TIP, IDS_TIPS, MB_ICONQUESTION | MB_YESNO))
    {
        m_session->refreshContent();
    }
    return -1;
}

LRESULT MainWin::onMouseHide(UINT message, WPARAM wParam, LPARAM lParam)
{
    static MainWin* _this = this;

    if (m_hMouseHook)
    {
        UnhookWindowsHookEx(m_hMouseHook);
        m_hMouseHook = NULL;
        _this = NULL;
    }
    else
    {
        _this = this;
        m_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, [](int nCode, WPARAM wParam, LPARAM lParam)->LRESULT
            {
                if (nCode == HC_ACTION)
                {
                    if (WM_MOUSEMOVE == wParam)
                    {
                        MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
                        if (pMouseStruct != NULL)
                        {
                            POINT pt = pMouseStruct->pt;
                            RECT rc;
                            GetWindowRect(_this->m_hWnd, &rc);
                            if (PtInRect(&rc, pt))
                            {
                                switch (_this->m_data.keyset.mouseBackAction)
                                {
                                case MouseBackAction::NOTHING:
                                    break;
                                case MouseBackAction::RESTORE:
                                    if (!IsWindowVisible(_this->m_hWnd))
                                    {
                                        // show window
                                        _this->onHideWindow(WH_MOUSE_LL, wParam, lParam);
                                    }
                                    break;
                                case MouseBackAction::RESTORE_WITH_CTRL:
                                    if (!IsWindowVisible(_this->m_hWnd) && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
                                    {
                                        // show window with ctrl
                                        _this->onHideWindow(WH_MOUSE_LL, wParam, lParam);
                                    }
                                    break;
                                default:
                                    break;
                                }
                            }
                            else
                            {
                                // hide window
                                if (IsWindowVisible(_this->m_hWnd)) _this->onHideWindow(WH_MOUSE_LL, wParam, lParam);
                            }
                        }
                    }
                }
                return CallNextHookEx(NULL, nCode, wParam, lParam);
            }, m_hInstance, 0);
    }
    return TRUE;
}

void MainWin::playLoading()
{
    if (!m_imgLoading)
    {
        m_imgLoading = IRenderer::instance()->loadImageFromResource(m_hInstance, IDR_GIF_LOADING, L"GIF");
    }
    KillTimer(m_hWnd, IDT_PLAYLOADING);
    SetTimer(m_hWnd, IDT_PLAYLOADING, m_imgLoading->frameDelay(m_imgLoading->frameIndex()), NULL);
    redrawWindow();
}

void MainWin::stopLoading()
{
    KillTimer(m_hWnd, IDT_PLAYLOADING);
    redrawWindow();
}

void MainWin::startAutoPage()
{
    if (!m_autoPageEnabled)
    {
        m_autoPageEnabled = true;
        m_autoPagePaused = false;
        applyAutoPageTimer();
        updateStatusBar();
    }
}

void MainWin::stopAutoPage()
{
    if (m_autoPageEnabled)
    {
        m_autoPageEnabled = false;
        m_autoPagePaused = false;
        KillTimer(m_hWnd, IDT_AUTOPAGING);
        updateStatusBar();
    }
}

void MainWin::pauseAutoPage()
{
    if (m_autoPageEnabled && !m_autoPagePaused)
    {
        m_autoPagePaused = true;
        KillTimer(m_hWnd, IDT_AUTOPAGING);
        updateStatusBar();
    }
}

void MainWin::resumeAutoPage()
{
    if (m_autoPageEnabled && m_autoPagePaused)
    {
        m_autoPagePaused = false;
        applyAutoPageTimer();
        updateStatusBar();
    }
}

void MainWin::applyAutoPageTimer()
{
    KillTimer(m_hWnd, IDT_AUTOPAGING);

    if (!m_autoPageEnabled || m_autoPagePaused || !m_session->isValid() || m_session->isEnd())
    {
        return;
    }

    UINT uElapse = 0;
    if (AutoPageIntervalMode::FIXED_TIME == m_data.paging.intervalMode)
    {
        uElapse = m_data.paging.intervalValue;
    }
    else
    {
        uElapse = m_session->pageLength() * m_data.paging.intervalValue;
    }

    if (uElapse > 0)
    {
        SetTimer(m_hWnd, IDT_AUTOPAGING, uElapse, NULL);
    }
}

void MainWin::openFindDialog()
{
    const int FIND_WHAT_LEN = 256;
    static wchar_t szFindWhat[FIND_WHAT_LEN] = {0};
    static FINDREPLACE fr = {0};

    closeFindDialog();

    ZeroMemory(&fr, sizeof(fr));
    fr.lStructSize = sizeof(fr);
    fr.hwndOwner = m_hWnd;
    fr.hInstance = m_hInstance;
    fr.lpstrFindWhat = szFindWhat;
    fr.wFindWhatLen = FIND_WHAT_LEN;
    fr.Flags = FR_DOWN | FR_ENABLEHOOK;
    fr.lpfnHook = (LPFRHOOKPROC)[](HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)->UINT_PTR {
        switch (uMsg)
        {
        case WM_INITDIALOG:
        {
            HWND hwndOwner = GetWindow(hwndDlg, GW_OWNER);
            if (hwndOwner)
            {
                EnableWindow(hwndOwner, FALSE);
            }
            return TRUE;
        }
        default: break;
        }
        return FALSE;
    };

    m_hFindDlg = FindText(&fr);
}

void MainWin::closeFindDialog()
{
    if (IsWindow(m_hFindDlg))
    {
        DestroyWindow(m_hFindDlg);
        m_hFindDlg = NULL;
    }
}

void MainWin::minToTray(bool bShow)
{
    if (bShow)
    {
        if (!m_nid.hWnd)
        {
            m_nid.hWnd = m_hWnd;
            m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            m_nid.uID = IDM_SYSTRAY;
            m_nid.uCallbackMessage = WM_SYSTRAY;
            m_nid.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_READER));
            wcscpy_s(m_nid.szTip, loadString(IDS_APP_TITLE).c_str());
            Shell_NotifyIcon(NIM_ADD, &m_nid);
        }
    }
    else
    {
        if (m_nid.hWnd)
        {
            Shell_NotifyIcon(NIM_DELETE, &m_nid);
            m_nid.hWnd = nullptr;
        }
    }
}

void MainWin::hideTaskbar(bool bHide)
{
    ITaskbarList* pTaskbarList = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList, (void**)&pTaskbarList);
    if (SUCCEEDED(hr))
    {
        pTaskbarList->HrInit();
        if (bHide)
        {
            pTaskbarList->DeleteTab(m_hWnd);
        }
        else
        {
            pTaskbarList->AddTab(m_hWnd);
        }
        pTaskbarList->Release();
    }
}

void MainWin::registerHotKey(bool bReg)
{
    if (bReg)
    {
        for (auto& k : m_data.keyset.keys)
        {
            if (KeyType::HOTKEY == k.type)
            {
                if (k.enable && !RegisterHotKey(m_hWnd, (int)k.id, HIBYTE(k.value), LOBYTE(k.value)))
                {
                    messageBoxFmt(IDS_REGISTER_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK, loadString(k.nameId).c_str());
                    onOpenKeySettingDlg(0, 0, 0);
                }
            }
        }
    }
    else
    {
        for (auto& k : m_data.keyset.keys)
        {
            if (KeyType::HOTKEY == k.type)
            {
                UnregisterHotKey(m_hWnd, (int)k.id);
            }
        }
    }
}

void MainWin::initTimers()
{
#ifdef ENABLE_NETWORK
    SetTimer(m_hWnd, IDT_UPGRADE, CHECK_VERSION_TIMEOUT * 1000, NULL);
    SetTimer(m_hWnd, IDT_CHECKBOOK, CHECK_BOOK_TIMEOUT * 1000, NULL);
#endif
}

void MainWin::redrawWindow()
{
    if (DisplayMode::NORMAL == m_data.window.mode)
    {
#if 0
        static bool isMenuDrawed = false;
        UINT flags = RDW_INVALIDATE | RDW_UPDATENOW;
        if (!isMenuDrawed)
        {
            isMenuDrawed = true;
            flags |= RDW_FRAME;
        }
        RedrawWindow(m_hWnd, NULL, NULL, flags);
#else
        InvalidateRect(m_hWnd, NULL, FALSE);
#endif
    }
    else
    {
        onPaint(nullptr);
    }
}

void MainWin::showWindow(bool bShow)
{
    if (bShow)
    {
        if (!IsWindowVisible(m_hWnd))
        {
            ShowWindow(m_hWnd, SW_SHOW);
        }
        if (IsIconic(m_hWnd))
        {
            ShowWindow(m_hWnd, SW_RESTORE);
        }
        SetForegroundWindow(m_hWnd);
    }
    else
    {
        if (IsWindowVisible(m_hWnd))
        {
            ShowWindow(m_hWnd, SW_HIDE);
            stopAutoPage();
        }
    }
}

void MainWin::saveWindowPlacement()
{
    if (m_isSwitching || m_isCreating)
        return;
    if (DisplayMode::FULLSCREEN == m_data.window.mode)
    {
        return;
    }
    if (DisplayMode::BORDERLESS == m_data.window.mode)
    {
        GetWindowRect(m_hWnd, &m_data.window.borderlessRect);
        m_data.window.placement.showCmd = SW_NORMAL;
        m_data.window.placement.flags &= ~WPF_SETMINPOSITION;
        m_data.window.placement.rcNormalPosition = calcNormalRect();
    }
    else
    {
        GetWindowPlacement(m_hWnd, &m_data.window.placement);
        m_data.window.borderlessRect = calcBorderlessRect();
    }
    Settings::instance().save(300);
}

void MainWin::switchDisplayMode(DisplayMode mode)
{
    m_isSwitching = true;
    SendMessage(m_hWnd, WM_SETREDRAW, FALSE, 0);
    SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, (DWORD)GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) & (~WS_EX_LAYERED));
    if (DisplayMode::NORMAL == mode)
    {
        m_data.window.style = WS_OVERLAPPEDWINDOW;
        m_data.window.exStyle = (DWORD)GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED;
        m_data.window.mode = DisplayMode::NORMAL;

        SetWindowLongPtr(m_hWnd, GWL_STYLE, m_data.window.style);
        SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, m_data.window.exStyle);
        ShowWindow(m_hStatusBar, SW_SHOW);
        SetMenu(m_hWnd, m_hMenu);
        SetLayeredWindowAttributes(m_hWnd, 0, m_data.window.alpha < MIN_LAYERED_ALPHA ? MIN_LAYERED_ALPHA : m_data.window.alpha, LWA_ALPHA);
        SetWindowPlacement(m_hWnd, &m_data.window.placement);
    }
    else if (DisplayMode::BORDERLESS == mode)
    {
        m_data.window.borderlessRect = calcBorderlessRect();
        const RECT& rcWin = m_data.window.borderlessRect;

        m_data.window.style = WS_POPUP;
        m_data.window.exStyle = (DWORD)GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED;
        m_data.window.mode = DisplayMode::BORDERLESS;

        SetWindowLongPtr(m_hWnd, GWL_STYLE, m_data.window.style);
        SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, m_data.window.exStyle);
        ShowWindow(m_hStatusBar, SW_HIDE);
        SetMenu(m_hWnd, NULL);
        SetWindowPos(m_hWnd, HWND_TOP, rcWin.left, rcWin.top, rcWin.right - rcWin.left, rcWin.bottom - rcWin.top, SWP_NOREDRAW | SWP_NOACTIVATE | SWP_NOCOPYBITS);
    }
    else if (DisplayMode::FULLSCREEN == mode)
    {
        MONITORINFO mi = { sizeof(MONITORINFO) };
        GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

        m_data.window.mode = DisplayMode::FULLSCREEN;

        SetWindowLongPtr(m_hWnd, GWL_STYLE, WS_POPUP);
        SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, (DWORD)GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        ShowWindow(m_hStatusBar, SW_HIDE);
        SetMenu(m_hWnd, NULL);
        SetWindowPos(m_hWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOREDRAW | SWP_NOACTIVATE | SWP_NOCOPYBITS);
    }
    redrawWindow();
    m_isSwitching = false;
    SendMessage(m_hWnd, WM_SETREDRAW, TRUE, 0);
    Settings::instance().save();
}

RECT MainWin::drawTextRect()
{
    RECT rc;
    GetClientRect(m_hWnd, &rc);
    if (DisplayMode::NORMAL == m_data.window.mode)
    {
        RECT rcStatus;
        GetWindowRect(m_hStatusBar, &rcStatus);
        rc.bottom -= rcStatus.bottom - rcStatus.top;
    }
    return rc;
}

RECT MainWin::calcNormalRect()
{
    if (DisplayMode::BORDERLESS == m_data.window.mode)
    {
        RECT rcWin;
        GetClientRect(m_hWnd, &rcWin);
        RECT rcStatus;
        GetWindowRect(m_hStatusBar, &rcStatus);
        rcWin.bottom += rcStatus.bottom - rcStatus.top;
        AdjustWindowRectEx(&rcWin, WS_OVERLAPPEDWINDOW, FALSE, (DWORD)GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        rcWin.top -= menuHeight(rcWin.right - rcWin.left);
        ClientToScreen(m_hWnd, reinterpret_cast<POINT *>(&rcWin.left));
        ClientToScreen(m_hWnd, reinterpret_cast<POINT *>(&rcWin.right));
        return rcWin;
    }
    else
    {
        return m_data.window.placement.rcNormalPosition;
    }
}

RECT MainWin::calcBorderlessRect()
{
    if (DisplayMode::NORMAL == m_data.window.mode)
    {
        RECT rcCli = drawTextRect();
        ClientToScreen(m_hWnd, reinterpret_cast<POINT*>(&rcCli.left));
        ClientToScreen(m_hWnd, reinterpret_cast<POINT*>(&rcCli.right));
        return rcCli;
    }
    else
    {
        return m_data.window.borderlessRect;
    }
}

int MainWin::menuHeight(int width)
{
    if (!m_hMenu)
    {
        return GetSystemMetrics(SM_CYMENU);
    }

    MENUBARINFO mbi;
    mbi.cbSize = sizeof(mbi);
    if (GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi))
    {
        return mbi.rcBar.bottom - mbi.rcBar.top;
    }

    // this is not a good way to get menu height, but it easily works
    // "dummy window" is bebetter way to get menu height, SetMenu(m_hDummy, m_hMenu)
    width = max(GetSystemMetrics(SM_CXMINTRACK), width);
    int itemCount = GetMenuItemCount(m_hMenu);
    if (itemCount <= 0)
    {
        return GetSystemMetrics(SM_CYMENU);
    }

    RECT rcItem;
    int h = 0;
    int w = 0;
    for (int i=0; i<itemCount; i++)
    {
        GetMenuItemRect(m_hWnd, m_hMenu, i, &rcItem);
        if (w + rcItem.right-rcItem.left > width && w > 0)
        {
            h += rcItem.bottom - rcItem.top;
            w = rcItem.right-rcItem.left;
        }
        else
        {
            w += rcItem.right-rcItem.left;
        }
    }
    return h + GetSystemMetrics(SM_CYMENU);
}

void MainWin::initHttps()
{
#ifdef ENABLE_NETWORK
    https_init();
    https_set_logger(lgprint);
    setHttpsParam();
#endif
}

void MainWin::setHttpsParam()
{
#ifdef ENABLE_NETWORK
    std::string ipaddr = Utils::adapterIp(m_data.network.adapterName);
    https_set_local_ip(ipaddr.c_str());

    char* addr = nullptr;
    conv_utf16_to_ansi(m_data.network.addr.c_str(), (int)m_data.network.addr.size(), &addr);
    char* user = nullptr;
    conv_utf16_to_ansi(m_data.network.user.c_str(), (int)m_data.network.user.size(), &user);
    char* pass = nullptr;
    conv_utf16_to_ansi(m_data.network.pass.c_str(), (int)m_data.network.pass.size(), &pass);

    http_proxy_t proxy = { 0 };
    proxy.enable = m_data.network.enableProxy;
    proxy.addr = addr;
    proxy.user = user;
    proxy.pass = pass;
    proxy.port = m_data.network.port;
    https_set_proxy(&proxy);

    conv_free(addr);
    conv_free(user);
    conv_free(pass);
#endif
}

void MainWin::deleteMenu(HMENU hMenu, UINT id, BOOL separator)
{
    int count = GetMenuItemCount(hMenu);
    for (int i = 0; i < count; i++)
    {
        HMENU hSubMenu = GetSubMenu(hMenu, i);
        if (hSubMenu)
            deleteMenu(hSubMenu, id, separator);
        if (id == GetMenuItemID(hMenu, i))
        {
            DeleteMenu(hMenu, i, MF_BYPOSITION);
            if (separator)
            {
                MENUITEMINFO mii = { sizeof(mii) };
                mii.fMask = MIIM_FTYPE;
                if (GetMenuItemInfo(hMenu, i, TRUE, &mii) && (mii.fType == MFT_SEPARATOR))
                {
                    DeleteMenu(hMenu, i, MF_BYPOSITION);
                }
            }
        }
    }
}

void MainWin::enableMenu(HMENU hMenu, BOOL enabled)
{
    if (!hMenu)
        return;

    int count = GetMenuItemCount(hMenu);
    for (int i = 0; i < count; i++)
    {
        HMENU hSubMenu = GetSubMenu(hMenu, i);
        if (hSubMenu) enableMenu(hSubMenu, enabled);
        int menuid = GetMenuItemID(hMenu, i);
        if (-1 != menuid) EnableMenuItem(hMenu, menuid, enabled ? MF_ENABLED : MF_DISABLED);
    }
    DrawMenuBar(m_hWnd);
}

void MainWin::updateTitle()
{
    std::wstring title = loadString(IDS_APP_TITLE);
    if (m_session->book())
    {
        title += L" - ";
        title += m_session->book()->bookName();
        if (m_session->isValid())
        {
            auto* chapter = m_session->book()->chapter(m_session->book()->curChapterIndex());
            if (chapter)
            {
                title += L" - ";
                title += chapter->title;
            }
        }
    }
    SetWindowTextW(m_hWnd, title.c_str());
}

void MainWin::updateStatusBar()
{
    std::wstring text = L"";
    if (m_hEditCtrl && m_hEditCtrl->isEditing())
    {
        text = loadString(IDS_EDIT_MODE_TIPS);
    }
    else
    {
        if (m_session->book())
        {
            text += m_session->progressStr();
            if (m_autoPageEnabled)
            {
                if (m_autoPagePaused)
                {
                    text += L"  [" + loadString(IDS_AUTOPAGINGPAUSED) + L"]";
                }
                else
                {
                    text += L"  [" + loadString(IDS_AUTOPAGING) + L"]";
                }
            }
        }
        else
        {
            text = L"";
        }
    }
    SendMessage(m_hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)text.c_str());
}

void MainWin::updateTreeViewFont()
{
    static HFONT curHFont = nullptr;
    static LOGFONTW curFont = {0};

    LOGFONTW newFont = {0};
    int iMenuHeight;

    // use text font with tree view
    if (m_data.styles.viewFont)
    {
        newFont = m_data.styles.textFont;
        iMenuHeight = abs((int)(newFont.lfHeight * 1.5f));
    }
    else
    {
        NONCLIENTMETRICS theMetrics = {0};
        theMetrics.cbSize = sizeof(NONCLIENTMETRICS);

#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0A00)
        SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), (PVOID)&theMetrics, 0, DpiAwareness::instance().dpi());
#else
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), (PVOID)&theMetrics, 0);
#endif
        newFont = theMetrics.lfMenuFont;
        iMenuHeight = abs((int)(theMetrics.iMenuHeight * 1.25f)); // 1.25 line height
    }

    if (memcmp(&curFont, &newFont, sizeof(LOGFONTW)) == 0) return;
    if (iMenuHeight == 0) iMenuHeight = 20;

    HFONT newHFont = CreateFontIndirectW(&newFont);
    if (!newHFont) return;

    HFONT oldFont = curHFont;
    curHFont = newHFont;
    curFont = newFont;

    SendMessageW(m_hTreeView, WM_SETFONT, (WPARAM)curHFont, TRUE);
    SendMessageW(m_hTreeView, TVM_SETITEMHEIGHT, iMenuHeight, NULL);
    SendMessageW(m_hTreeMark, WM_SETFONT, (WPARAM)curHFont, TRUE);
    SendMessageW(m_hTreeMark, TVM_SETITEMHEIGHT, iMenuHeight, NULL);

    if (oldFont) DeleteObject(oldFont);
}

DWORD MainWin::toModifiers(WPARAM wParam)
{
    UINT modifiers = 0;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)  modifiers |= MOD_CONTROL;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)    modifiers |= MOD_SHIFT;
    if (GetAsyncKeyState(VK_MENU) & 0x8000)     modifiers |= MOD_ALT;
    return MAKEWORD(wParam, modifiers);
}

void  MainWin::unloadBgImage()
{
    if (m_imgBackground)
    {
        delete m_imgBackground;
        m_imgBackground = nullptr;
    }
}

Image* MainWin::loadBgImage(int w, int h, BYTE alpha)
{
    static std::wstring _fileName;
    static int _width = 0;
    static int _height = 0;
    static BYTE _alpha = 0;
    static WrapMode _mode = WrapMode::TILEFLIP;

    auto& background = m_data.background;
    if (!background.enableImage)
    {
        return nullptr;
    }

    if (m_imgBackground && background.fileName == _fileName && w == _width && h == _height && alpha == _alpha && background.mode == _mode)
    {
        return m_imgBackground;
    }

    _width = w;
    _height = h;
    _alpha = alpha;
    _fileName = background.fileName;
    _mode = background.mode;

    Image* temp = IRenderer::instance()->loadImageFromPath(_fileName.c_str());
    if (temp)
    {
        unloadBgImage();
        m_imgBackground = IRenderer::instance()->createImageFromBGImage(temp, _alpha, _width, _height, _mode);
        delete temp;
    }
    return m_imgBackground;
}