#ifdef ENABLE_NETWORK
#include "NetworkSettingDlg.h"
#include "resource.h"
#include "Settings.h"

NetworkSettingDlg::NetworkSettingDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_NETWORKSETTING)
    , m_changed(false)
{
}

NetworkSettingDlg::~NetworkSettingDlg()
{
}

bool NetworkSettingDlg::isChanged() const
{
    return m_changed;
}

LRESULT NetworkSettingDlg::onInitDialog()
{
    auto& network = Settings::instance().data().network;
    m_adapters = Utils::enumNetworkAdapters();

    // network adapter settings
    int index = 0;
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_ADAPTERS), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_DEFAULT_NETWORK_ADAPTER).c_str());
    for (int i=0; i < (int)m_adapters.size(); i++)
    {
        std::wstring item = m_adapters[i].name + L" - " + std::wstring(m_adapters[i].ip.begin(), m_adapters[i].ip.end()) + L"";
        SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_ADAPTERS), CB_ADDSTRING, 0, (LPARAM)item.c_str());

        if (m_adapters[i].name == network.adapterName)
        {
            index = i + 1;
        }
    }
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_ADAPTERS), CB_SETCURSEL, index, NULL);

    // proxy settings
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_PROXY), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_DISABLE_PROXY).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_PROXY), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_ENABLE_PROXY).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_PROXY), CB_SETCURSEL, network.enableProxy ? 1 : 0, NULL);
    SetDlgItemTextW(m_hWnd, IDC_EDIT_PROXY_ADDR, network.addr.c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_PROXY_USER, network.user.c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_PROXY_PASS, network.pass.c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_PROXY_PORT, network.port == 0 ? L"" : std::to_wstring(network.port).c_str());
    enableProxySettings(network.enableProxy);

    centerParent();
    return TRUE;
}

LRESULT NetworkSettingDlg::onSave()
{
    bool enableProxy = (int)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_PROXY), CB_GETCURSEL, 0, NULL) == 1;
    std::wstring addr = getDlgItemTextW(IDC_EDIT_PROXY_ADDR);
    std::wstring port = getDlgItemTextW(IDC_EDIT_PROXY_PORT);
    std::wstring user = getDlgItemTextW(IDC_EDIT_PROXY_USER);
    std::wstring pass = getDlgItemTextW(IDC_EDIT_PROXY_PASS);
    if (enableProxy)
    {
        if (addr.empty())
        {
            messageBox(IDS_PROXY_ADDR_EMPTY, IDS_WARN, MB_ICONWARNING | MB_OK);
            SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_PROXY_ADDR));
            return FALSE;
        }
        if (port.empty())
        {
            messageBox(IDS_PROXY_PORT_EMPTY, IDS_WARN, MB_ICONWARNING | MB_OK);
            SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_PROXY_PORT));
            return FALSE;
        }
        int portNum = _wtoi(port.c_str());
        if (portNum <= 0 || portNum > 65535)
        {
            messageBox(IDS_PROXY_PORT_INVALID, IDS_WARN, MB_ICONWARNING | MB_OK);
            SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_PROXY_PORT));
            return FALSE;
        }
        if (!user[0] && pass[0])
        {
            messageBox(IDS_PROXY_USER_EMPTY, IDS_WARN, MB_ICONWARNING | MB_OK);
            SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_PROXY_USER));
            return FALSE;
        }
        if (user[0] && !pass[0])
        {
            messageBox(IDS_PROXY_PASS_EMPTY, IDS_WARN, MB_ICONWARNING | MB_OK);
            SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_PROXY_PASS));
            return FALSE;
        }
    }

    auto& network = Settings::instance().data().network;
    if (network.enableProxy != enableProxy)
    {
        network.enableProxy = enableProxy;
        m_changed = true;
    }
    if (network.enableProxy
        && (network.addr != addr || network.port != _wtoi(port.c_str()) || network.user != user || network.pass != pass))
    {
        m_changed = true;
    }
    network.addr = addr;
    network.port = _wtoi(port.c_str());
    network.user = user;
    network.pass = pass;

    int adapterIndex = (int)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_ADAPTERS), CB_GETCURSEL, 0, NULL);
    std::wstring selectedAdapterName = L"";
    if (adapterIndex > 0)
    {
        selectedAdapterName = m_adapters[adapterIndex - 1].name;
    }
    if (network.adapterName != selectedAdapterName)
    {
        network.adapterName = selectedAdapterName;
        m_changed = true;
    }
    Settings::instance().save();

    return EndDialog(m_hWnd, IDOK);
}

void NetworkSettingDlg::enableProxySettings(bool enable)
{
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_PROXY_ADDR), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_PROXY_USER), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_PROXY_PASS), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_PROXY_PORT), enable);
}

LRESULT NetworkSettingDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return onInitDialog();

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            return onSave();
        case IDCANCEL:
            EndDialog(m_hWnd, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDC_COMBO_PROXY:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int sel = (int)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_PROXY), CB_GETCURSEL, 0, NULL);
                enableProxySettings(sel == 1);
            }
            break;
        default:
            break;
        }

    default:
        break;
    }
    return (INT_PTR)FALSE;
}
#endif