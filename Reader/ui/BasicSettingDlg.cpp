#include "BasicSettingDlg.h"
#include "resource.h"

BasicSettingDlg::BasicSettingDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_BASICSETTING)
    , m_isMinToTrayChanged(false)
    , m_isHideTaskbarChanged(false)
{
}

BasicSettingDlg::~BasicSettingDlg()
{
}

bool BasicSettingDlg::isMinToTrayChanged() const
{
    return m_isMinToTrayChanged;
}

bool BasicSettingDlg::isHideTaskbarChanged() const
{
    return m_isHideTaskbarChanged;
}

LRESULT BasicSettingDlg::onInitDialog()
{
    auto& data = Settings::instance().data();

    // page settings
    SetDlgItemInt(m_hWnd, IDC_EDIT_PRESERVELINES, data.paging.preserveLines, FALSE);

    if (MousePageMode::DISABLED == data.paging.mouseMode)
        CheckRadioButton(m_hWnd, IDC_RADIO_MODE1, IDC_RADIO_MODE3, IDC_RADIO_MODE1);
    else if (MousePageMode::CLICK_ANYWHERE == data.paging.mouseMode)
        CheckRadioButton(m_hWnd, IDC_RADIO_MODE1, IDC_RADIO_MODE3, IDC_RADIO_MODE2);
    else if (MousePageMode::CLICK_SIDES == data.paging.mouseMode)
        CheckRadioButton(m_hWnd, IDC_RADIO_MODE1, IDC_RADIO_MODE3, IDC_RADIO_MODE3);

    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_SCROLLMODE), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_LINE).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_SCROLLMODE), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_PAGE).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_SCROLLMODE), CB_SETCURSEL, (int)data.paging.scrollMode, NULL);

    // auto page settings
    SetDlgItemInt(m_hWnd, IDC_EDIT_ELAPSE, data.paging.intervalValue, FALSE);
    if (AutoPageMode::PAGE_FLIP == data.paging.autoMode)
        CheckRadioButton(m_hWnd, IDC_RADIO_ATPAGE, IDC_RADIO_ATWHEEL, IDC_RADIO_ATPAGE);
    else if (AutoPageMode::LINE_SCROLL == data.paging.autoMode)
        CheckRadioButton(m_hWnd, IDC_RADIO_ATPAGE, IDC_RADIO_ATWHEEL, IDC_RADIO_ATWHEEL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_APM), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_FIXED_TIME).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_APM), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_DYNAMIC_CALC).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_APM), CB_SETCURSEL, (int)data.paging.intervalMode, NULL);

    // window settings
    CheckDlgButton(m_hWnd, IDC_CHECK_TRAY, data.window.minToTray ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hWnd, IDC_CHECK_TASKBAR, data.window.hideTaskbar ? BST_CHECKED : BST_UNCHECKED);

    centerParent();
    return TRUE;
}

LRESULT BasicSettingDlg::onEndDialog(INT_PTR nResult)
{
    if (nResult == IDOK)
    {
        auto& data = Settings::instance().data();
        // page settings
        data.paging.preserveLines = GetDlgItemInt(m_hWnd, IDC_EDIT_PRESERVELINES, NULL, FALSE);
        data.paging.scrollMode = (ScrollMode)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_SCROLLMODE), CB_GETCURSEL, 0, NULL);
        if (IsDlgButtonChecked(m_hWnd, IDC_RADIO_MODE1))
            data.paging.mouseMode = MousePageMode::DISABLED;
        else if (IsDlgButtonChecked(m_hWnd, IDC_RADIO_MODE2))
            data.paging.mouseMode = MousePageMode::CLICK_ANYWHERE;
        else if (IsDlgButtonChecked(m_hWnd, IDC_RADIO_MODE3))
            data.paging.mouseMode = MousePageMode::CLICK_SIDES;

        // auto page settings
        data.paging.intervalValue = GetDlgItemInt(m_hWnd, IDC_EDIT_ELAPSE, NULL, FALSE);
        data.paging.intervalMode = (AutoPageIntervalMode)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_APM), CB_GETCURSEL, 0, NULL);
        if (IsDlgButtonChecked(m_hWnd, IDC_RADIO_ATPAGE))
            data.paging.autoMode = AutoPageMode::PAGE_FLIP;
        else if (IsDlgButtonChecked(m_hWnd, IDC_RADIO_ATWHEEL))
            data.paging.autoMode = AutoPageMode::LINE_SCROLL;

        // window settings
        m_isMinToTrayChanged = data.window.minToTray != (IsDlgButtonChecked(m_hWnd, IDC_CHECK_TRAY) == BST_CHECKED);
        m_isHideTaskbarChanged = data.window.hideTaskbar != (IsDlgButtonChecked(m_hWnd, IDC_CHECK_TASKBAR) == BST_CHECKED);
        data.window.minToTray = IsDlgButtonChecked(m_hWnd, IDC_CHECK_TRAY) == BST_CHECKED;
        data.window.hideTaskbar = IsDlgButtonChecked(m_hWnd, IDC_CHECK_TASKBAR) == BST_CHECKED;

        Settings::instance().save();
    }
    EndDialog(m_hWnd, nResult);
    return (INT_PTR)TRUE;
}

LRESULT BasicSettingDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
            return onEndDialog(LOWORD(wParam));
        default:
            break;
        }
        break;

    default:
        break;
    }
    return (INT_PTR)FALSE;
}