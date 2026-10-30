#include "AboutDlg.h"
#include "resource.h"
#include "Utils.h"
#include "RewardDlg.h"
#include "FollowDlg.h"

AboutDlg::AboutDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_ABOUTBOX)
{
}

AboutDlg::~AboutDlg()
{
}

LRESULT AboutDlg::onInitDialog()
{
    auto fReplaceText = [this](HWND hWnd, UINT id, const wchar_t* flag, const std::wstring& value)
        {
            std::wstring text = getDlgItemTextW(id);
            Utils::replace(text, flag, value);
            SetDlgItemTextW(hWnd, id, text.c_str());
        };

    fReplaceText(m_hWnd, IDC_STATIC_VERSION, L"{VERSION}", Utils::appVersion());
    fReplaceText(m_hWnd, IDC_STATIC_COPYRIGHT, L"{YEAR}", Utils::currentYear());
    fReplaceText(m_hWnd, IDC_STATIC_LINK, L"{SOURCE}", SOURCE_ADDRESS);
    centerParent();
    return TRUE;
}

LRESULT AboutDlg::onReward()
{
    RewardDlg dlg(m_hInstance, m_hWnd);
    return dlg.exec();
}

LRESULT AboutDlg::onFollow()
{
    FollowDlg dlg(m_hInstance, m_hWnd);
    return dlg.exec();
}

LRESULT AboutDlg::onLButtonDown(WPARAM wParam, LPARAM lParam)
{
    POINT pt = { LOWORD(lParam), HIWORD(lParam) };
    HWND hWnd = GetDlgItem(m_hWnd, IDC_STATIC_LINK);
    ClientToScreen(m_hWnd, &pt);
    RECT rc;
    GetWindowRect(hWnd, &rc);
    if (PtInRect(&rc, pt))
    {
#ifdef ENABLE_NETWORK
        ShellExecuteW(NULL, L"open", SOURCE_ADDRESS, NULL, NULL, SW_SHOWNORMAL);
#endif
    }
    return TRUE;
}

LRESULT AboutDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return onInitDialog();

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(m_hWnd, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDC_BUTTON_REWARD:
            return onReward();
        case IDC_BUTTON_FOLLOW:
            return onFollow();
        }
        break;

    case WM_LBUTTONDOWN:
        return onLButtonDown(wParam, lParam);

    default:
        break;
    }
    return (INT_PTR)FALSE;
}