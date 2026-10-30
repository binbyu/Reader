#ifdef ENABLE_NETWORK
#include "UpgradeDlg.h"
#include "resource.h"
#include "https.h"
#include "conv.h"
#include "JsonSerialize.h"
#include "Utils.h"
#include <time.h>
#include <algorithm>

#define MAKEVER(a, b, c, d) MAKELONG(MAKEWORD(d, c), MAKEWORD(b, a))

UpgradeDlg::UpgradeDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_UPGRADE)
    , m_info(nullptr)
{
}

UpgradeDlg::~UpgradeDlg()
{
}

LRESULT UpgradeDlg::onInitDialog()
{
    if (m_info)
    {
        SetDlgItemTextW(m_hWnd, IDC_EDIT_UPGRADE_VERSION, m_info->version.c_str());
        SetDlgItemTextW(m_hWnd, IDC_EDIT_UPGRADE_DESC, m_info->description.c_str());
    }

    centerParent();
    return TRUE;
}

LRESULT UpgradeDlg::onIgnoreButtonClick()
{
    if (m_info)
    {
        Settings::instance().data().upgrade.ignoreVersion = m_info->version;
        Settings::instance().save();
    }
    EndDialog(m_hWnd, IDC_BUTTON_UPGRADE_INGORE);
    return TRUE;
}

LRESULT UpgradeDlg::onDownButtonClick()
{
    if (m_info && !m_info->downloadurl.empty())
    {
        ShellExecuteW(NULL, L"open", m_info->downloadurl.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    return TRUE;
}

void UpgradeDlg::checkVersion(HWND hParent)
{
    static VersionInfo _version;
    static request_handle_t _hRequest = nullptr;
    auto& upgrade = Settings::instance().data().upgrade;

    uint64_t curnTime = time(NULL);
    if (curnTime < upgrade.lastCheckTime + CHECK_VERSION_TIMEOUT)
    {
        return;
    }
    upgrade.lastCheckTime = curnTime;
    Settings::instance().save();

    if (_hRequest)
    {
        https_cancel(_hRequest);
        _hRequest = nullptr;
    }

    request_t req;
    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = (char*)VERSION_INFO_URL;
    req.param1 = hParent;
    req.param2 = NULL;
    req.completer = [](request_result_t* result) -> unsigned int {

        _hRequest = nullptr;
        HWND hWnd = (HWND)result->param1;
        if (result->errno_ == ECAN)
        {
            return 1;
        }
        if (result->errno_ != SUCC || result->status_code != 200)
        {
            return 1;
        }
        char* html = nullptr;
        int htmlLen = conv_to_utf8(result->body, result->bodylen, &html);
        if (!html)
        {
            return 1;
        }

        std::vector<uint8_t> bytes;
        bytes.assign(html, html + htmlLen);
        conv_free(html);

        VersionList vers;
        if (!JsonSerialize::deserialize(bytes, vers) || vers.list.empty())
        {
            return 1;
        }

        std::sort(vers.list.begin(), vers.list.end(), [](const VersionInfo& a, const VersionInfo& b) { return vercmp(a.version.c_str(), b.version.c_str()) > 0; });
        _version = vers.list[0];
        if (_version.version == Settings::instance().data().upgrade.ignoreVersion)
        {
            return 1;
        }
        std::wstring appVersion = Utils::appVersion();
        if (vercmp(_version.version.c_str(), appVersion.c_str()) <= 0)
        {
            return 1;
        }

        PostMessage(hWnd, WM_NEWVERSION, 0, (LPARAM)&_version);
        return 0;
    };

    _hRequest = https_request(&req);
}

void UpgradeDlg::setVersion(VersionInfo* info)
{
    m_info = info;
}

int UpgradeDlg::vercmp(const wchar_t* v1, const wchar_t* v2)
{
    unsigned int v1_1 = 0, v1_2 = 0, v1_3 = 0, v1_4 = 0;
    unsigned int v2_1 = 0, v2_2 = 0, v2_3 = 0, v2_4 = 0;
    swscanf(v1, L"%d.%d.%d.%d", &v1_1, &v1_2, &v1_3, &v1_4);
    swscanf(v2, L"%d.%d.%d.%d", &v2_1, &v2_2, &v2_3, &v2_4);

    unsigned int num1 = MAKEVER(v1_1, v1_2, v1_3, v1_4);
    unsigned int num2 = MAKEVER(v2_1, v2_2, v2_3, v2_4);
    return num1 == num2 ? 0 : (num1 > num2 ? 1 : -1);
}

LRESULT UpgradeDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
        case IDC_BUTTON_UPGRADE_INGORE:
            return onIgnoreButtonClick();
        case IDC_BUTTON_UPGRADE_DOWN:
            return onDownButtonClick();
        }
        break;

    default:
        break;
    }
    return (INT_PTR)FALSE;
}
#endif