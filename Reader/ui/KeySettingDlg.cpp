#include "KeySettingDlg.h"
#include "resource.h"
#include "Settings.h"

KeySettingDlg::KeySettingDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_KEYSETTING)
{
}

KeySettingDlg::~KeySettingDlg()
{
}

LRESULT KeySettingDlg::onInitDialog()
{
    ShowWindow(GetDlgItem(m_hWnd, IDC_CHECK), SW_HIDE);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_MOUSEHIDE), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_NOTHING).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_MOUSEHIDE), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_SHOW_WINDOW).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_MOUSEHIDE), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_SHOW_WINDOW_WITH_CTRL).c_str());

    auto& keyset = Settings::instance().data().keyset;
    initData(keyset);

    // Subclass hotkey controls to support VK_SPACE
    for (const auto& k : keyset.keys)
    {
        HWND hkCtrl = GetDlgItem(m_hWnd, k.hkCtrlId);
        SetWindowSubclass(hkCtrl, hotKeyProc, 0, 0);
    }

    centerParent();
    return TRUE;
}

LRESULT KeySettingDlg::onRestoreDefault()
{
    if (IDYES != messageBox(IDS_RESTORE_DEFAULT, IDS_TIPS, MB_ICONINFORMATION | MB_YESNO))
    {
        return FALSE;
    }

    KeysetInfo keyset;
    Settings::instance().defaultKeys(keyset);
    initData(keyset);
    return (INT_PTR)TRUE;
}

LRESULT KeySettingDlg::onSave()
{
    KeysetInfo keyset;
    Settings::instance().defaultKeys(keyset);
    loadData(keyset);
    if (!isValid(keyset))
    {
        return FALSE;
    }
    Settings::instance().data().keyset = keyset;
    Settings::instance().save();
    EndDialog(m_hWnd, IDOK);
    return (INT_PTR)TRUE;
}

LRESULT KeySettingDlg::onExit()
{
    EndDialog(m_hWnd, IDCANCEL);
    return (INT_PTR)TRUE;
}

void KeySettingDlg::initData(KeysetInfo& keyset)
{
    for (auto& k : keyset.keys)
    {
        HWND hkCtrl = GetDlgItem(m_hWnd, k.hkCtrlId);
        HWND ckCtrl = GetDlgItem(m_hWnd, k.ckCtrlId);
        SendMessage(hkCtrl, HKM_SETHOTKEY, modToHot(k.value), 0);
        if (KeyType::SHORTCUT == k.type)
        {
            SendMessage(hkCtrl, HKM_SETRULES, HKCOMB_A, 0);
        }
        SendMessage(ckCtrl, BM_SETCHECK, k.enable ? BST_CHECKED : BST_UNCHECKED, NULL);
        EnableWindow(hkCtrl, k.enable);
    }

    SendMessage(GetDlgItem(m_hWnd, IDC_CHECK_MOUSELRHIDE), BM_SETCHECK, keyset.enableMouseLRHide ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_CHECK_ESCHIDE), BM_SETCHECK, keyset.enableEscHide ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_MOUSEHIDE), CB_SETCURSEL, (int)keyset.mouseBackAction, NULL);
}

void KeySettingDlg::loadData(KeysetInfo& keyset)
{
    for (auto& k : keyset.keys)
    {
        HWND hkCtrl = GetDlgItem(m_hWnd, k.hkCtrlId);
        HWND ckCtrl = GetDlgItem(m_hWnd, k.ckCtrlId);
        k.value = hotToMod((DWORD)SendMessage(hkCtrl, HKM_GETHOTKEY, 0, NULL));
        k.enable = SendMessage(ckCtrl, BM_GETCHECK, 0, NULL) == BST_CHECKED;
    }
    keyset.enableMouseLRHide = SendMessage(GetDlgItem(m_hWnd, IDC_CHECK_MOUSELRHIDE), BM_GETCHECK, 0, NULL) == BST_CHECKED;
    keyset.enableEscHide = SendMessage(GetDlgItem(m_hWnd, IDC_CHECK_ESCHIDE), BM_GETCHECK, 0, NULL) == BST_CHECKED;
    keyset.mouseBackAction = (MouseBackAction)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_MOUSEHIDE), CB_GETCURSEL, 0, NULL);
}

bool KeySettingDlg::isExtendedKey(BYTE vk)
{
    switch (vk)
    {
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:   // Page Up
    case VK_NEXT:    // Page Down
    case VK_INSERT:
    case VK_DELETE:
    case VK_DIVIDE:  // number key
        return true;
    default:
        return false;
    }
}

bool KeySettingDlg::isValid(KeysetInfo& keyset)
{
    for (int i = 0; i < (int)keyset.keys.size(); i++)
    {
        auto& k = keyset.keys[i];
        if (k.enable)
        {
            // check empty
            if (0 == k.value)
            {
                messageBoxFmt(IDS_KEYVALUE_EMPTY, IDS_WARN, MB_ICONWARNING | MB_OK, loadString(k.nameId).c_str());
                SetFocus(GetDlgItem(m_hWnd, k.hkCtrlId));
                return false;
            }
        }
        // check duplicate
        for (int j = i + 1; j < (int)keyset.keys.size(); j++)
        {
            if (k.value == keyset.keys[j].value)
            {
                messageBoxFmt(IDS_KEYVALUE_DUPLICATE, IDS_WARN, MB_ICONWARNING | MB_OK, loadString(k.nameId).c_str());
                SetFocus(GetDlgItem(m_hWnd, k.hkCtrlId));
                return false;
            }
        }
        // try to register hotkey to detect invalid key combinations
        if (KeyType::HOTKEY == k.type)
        {
            auto& curn_k = Settings::instance().data().keyset.keys[i];
            //if (k.value != curn_k.value)
            {
                UnregisterHotKey(m_hParentWnd, (int)k.id);
                if (k.enable && !RegisterHotKey(m_hParentWnd, (int)k.id, HIBYTE(k.value), LOBYTE(k.value)))
                {
                    // restore old hotkey
                    RegisterHotKey(m_hParentWnd, (int)curn_k.id, HIBYTE(curn_k.value), LOBYTE(curn_k.value));
                    messageBoxFmt(IDS_REGISTER_FAIL, IDS_WARN, MB_ICONWARNING | MB_OK, loadString(k.nameId).c_str());
                    SetFocus(GetDlgItem(m_hWnd, k.hkCtrlId));
                    return false;
                }
            }
        }
    }
    return true;
}

DWORD KeySettingDlg::hotToMod(DWORD value)
{
    BYTE vk  = LOBYTE(value);
    BYTE hot = HIBYTE(value);
    BYTE mod = 0;
    if (isExtendedKey(vk))      hot &= ~HOTKEYF_EXT;
    if (hot & HOTKEYF_CONTROL)  mod |= MOD_CONTROL;
    if (hot & HOTKEYF_ALT)      mod |= MOD_ALT;
    if (hot & HOTKEYF_SHIFT)    mod |= MOD_SHIFT;
    if (hot & HOTKEYF_EXT)      mod |= MOD_WIN;
    return MAKEWORD(vk, mod);
}

DWORD KeySettingDlg::modToHot(DWORD value)
{
    BYTE vk  = LOBYTE(value);
    BYTE mod = HIBYTE(value);
    BYTE hot = 0;
    if (mod & MOD_CONTROL)  hot |= HOTKEYF_CONTROL;
    if (mod & MOD_ALT)      hot |= HOTKEYF_ALT;
    if (mod & MOD_SHIFT)    hot |= HOTKEYF_SHIFT;
    if (mod & MOD_WIN)      hot |= HOTKEYF_EXT;
    if (isExtendedKey(vk))  hot |= HOTKEYF_EXT;
    return MAKEWORD(vk, hot);
}

LRESULT KeySettingDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
            return onExit();
        case IDC_BUTTON_DEFAULT:
            return onRestoreDefault();
        }
        // enable/disable hotkey controls according to the checkbox
        for (const auto& k : Settings::instance().data().keyset.keys)
        {
            if (LOWORD(wParam) == k.ckCtrlId)
            {
                BOOL checked = SendMessage(GetDlgItem(m_hWnd, k.ckCtrlId), BM_GETCHECK, 0, NULL) == BST_CHECKED;
                EnableWindow(GetDlgItem(m_hWnd, k.hkCtrlId), checked);
                break;
            }
        }
        break;

    default:
        break;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK KeySettingDlg::hotKeyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (msg == WM_KEYDOWN && wParam == VK_SPACE)
    {
        BYTE mod = 0;
        if (GetKeyState(VK_CONTROL) & 0x8000) mod |= MOD_CONTROL;
        if (GetKeyState(VK_MENU) & 0x8000) mod |= MOD_ALT;
        if (GetKeyState(VK_SHIFT) & 0x8000) mod |= MOD_SHIFT;
        SendMessage(hwnd, HKM_SETHOTKEY, MAKEWORD(VK_SPACE, mod), 0);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}