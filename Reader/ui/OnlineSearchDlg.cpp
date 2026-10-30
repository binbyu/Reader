#ifdef ENABLE_NETWORK
#include "OnlineSearchDlg.h"
#include "resource.h"

OnlineSearchDlg::OnlineSearchDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_ONLINESEARCH)
    , m_page1(nullptr)
    , m_page2(nullptr)
    , m_tabIndex(0)
{
}

OnlineSearchDlg::~OnlineSearchDlg()
{
    if (m_page1)
        delete m_page1;
    if (m_page2)
        delete m_page2;
}

std::wstring OnlineSearchDlg::openFileName() const
{
    if (m_tabIndex == 0)
        return m_page1->openFileName();
    else
        return m_page2->openFileName();
}

LRESULT OnlineSearchDlg::onInitDialog()
{
    SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_READER)));

    m_page1 = new SourceSearchDlg(m_hInstance, m_hWnd);
    m_page2 = new SimpleSearchDlg(m_hInstance, m_hWnd);
    m_page1->create();
    m_page2->create();

    std::wstring title1 = loadString(IDS_SOURCE_MODE);
    std::wstring title2 = loadString(IDS_SIMPLE_MODE);

    HWND hTab = GetDlgItem(m_hWnd, IDC_TAB_SEARCH);
    TCITEM tie = { 0 };
    tie.mask = TCIF_TEXT;
    tie.pszText = (LPWSTR)title1.c_str();
    TabCtrl_InsertItem(hTab, 0, &tie);
    tie.pszText = (LPWSTR)title2.c_str();
    TabCtrl_InsertItem(hTab, 1, &tie);

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    onSize(rc.right - rc.left, rc.bottom - rc.top);
    m_page1->show(SW_SHOW);
    m_page2->show(SW_HIDE);

    centerParent();
    return TRUE;
}

LRESULT OnlineSearchDlg::onSize(int width, int height)
{
    const int BORDER = 0;
    RECT rc;
    rc.left = BORDER;
    rc.top = BORDER;
    rc.right = width - BORDER * 2;
    rc.bottom = height - BORDER * 2;

    HWND hTab = GetDlgItem(m_hWnd, IDC_TAB_SEARCH);
    SetWindowPos(hTab, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
    TabCtrl_AdjustRect(hTab, FALSE, &rc);

    SetWindowPos(m_page1->getHWND(), HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
    SetWindowPos(m_page2->getHWND(), HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
    return TRUE;
}

LRESULT OnlineSearchDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return onInitDialog();

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        //case IDOK:
        case IDCANCEL:
            m_tabIndex = TabCtrl_GetCurSel(GetDlgItem(m_hWnd, IDC_TAB_SEARCH));
            m_page1->close();
            m_page2->close();
            EndDialog(m_hWnd, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

    case WM_SIZE:
        return onSize(LOWORD(lParam), HIWORD(lParam));

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* pMinMaxInfo = (MINMAXINFO*)lParam;
        pMinMaxInfo->ptMinTrackSize.x = 480; // min width
        pMinMaxInfo->ptMinTrackSize.y = 340; // min height
        break;
    }

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->idFrom == IDC_TAB_SEARCH && pnmh->code == TCN_SELCHANGE)
        {
            int iSel = TabCtrl_GetCurSel(pnmh->hwndFrom);
            ShowWindow(m_page1->getHWND(), (iSel == 0) ? SW_SHOW : SW_HIDE);
            ShowWindow(m_page2->getHWND(), (iSel == 1) ? SW_SHOW : SW_HIDE);
            return TRUE;
        }
        break;
    }

    default:
        break;
    }
    return (INT_PTR)FALSE;
}
#endif