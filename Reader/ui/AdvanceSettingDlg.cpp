#include "AdvanceSettingDlg.h"
#include "resource.h"
#include "Settings.h"
#include <regex>

AdvanceSettingDlg::AdvanceSettingDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_ADVANCESETTING)
    , m_bChanged(false)
{
}

AdvanceSettingDlg::~AdvanceSettingDlg()
{
}

bool AdvanceSettingDlg::isChanged() const
{
    return m_bChanged;
}

LRESULT AdvanceSettingDlg::onInitDialog()
{
    auto& chapterRule = Settings::instance().data().chapterRule;
    switchRule(chapterRule.rule);
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_KEYWORD, chapterRule.keyword.c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_REGEX, chapterRule.regex.c_str());
    centerParent();
    return TRUE;
}

LRESULT AdvanceSettingDlg::onEndDialog(INT_PTR nResult)
{
    if (nResult == IDOK)
    {
        ChapterRule chapterRule;
        if (IsDlgButtonChecked(m_hWnd, IDC_RADIO_CPT_DEFAULT))
        {
            chapterRule.rule = ChapterRule::Rule::DEFAULT;
        }
        else if (IsDlgButtonChecked(m_hWnd, IDC_RADIO_CPT_KEYWORD))
        {
            chapterRule.rule = ChapterRule::Rule::KEYWORD;
        }
        else if (IsDlgButtonChecked(m_hWnd, IDC_RADIO_CPT_REGEX))
        {
            chapterRule.rule = ChapterRule::Rule::REGEX;
        }
        chapterRule.keyword = getDlgItemTextW(IDC_EDIT_CPT_KEYWORD);
        chapterRule.regex = getDlgItemTextW(IDC_EDIT_CPT_REGEX);
        if (!isValidRule(chapterRule))
        {
            return (INT_PTR)FALSE;
        }
        if (isRuleChanged(chapterRule))
        {
            m_bChanged = true;
        }
        Settings::instance().data().chapterRule = chapterRule;
        Settings::instance().save();
    }
    EndDialog(m_hWnd, nResult);
    return (INT_PTR)TRUE;
}

void AdvanceSettingDlg::switchRule(ChapterRule::Rule rule)
{
    switch (rule)
    {
    case ChapterRule::DEFAULT:
        SendMessage(GetDlgItem(m_hWnd, IDC_RADIO_CPT_DEFAULT), BM_SETCHECK, BST_CHECKED, NULL);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_KEYWORD), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_REGEX), FALSE);
        break;
    case ChapterRule::KEYWORD:
        SendMessage(GetDlgItem(m_hWnd, IDC_RADIO_CPT_KEYWORD), BM_SETCHECK, BST_CHECKED, NULL);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_KEYWORD), TRUE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_REGEX), FALSE);
        break;
    case ChapterRule::REGEX:
        SendMessage(GetDlgItem(m_hWnd, IDC_RADIO_CPT_REGEX), BM_SETCHECK, BST_CHECKED, NULL);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_KEYWORD), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_REGEX), TRUE);
        break;
    default:
        break;
    }
}

bool AdvanceSettingDlg::isValidRule(const ChapterRule& rule)
{
    if (rule.rule == ChapterRule::Rule::KEYWORD && rule.keyword.empty())
    {
        messageBox(IDS_EMPTY_KEYWORD, IDS_WARN, MB_OK|MB_ICONWARNING);
        SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_CPT_KEYWORD));
        return false;
    }
    if (rule.rule == ChapterRule::Rule::REGEX && rule.regex.empty())
    {
        messageBox(IDS_EMPTY_REGEX, IDS_WARN, MB_OK|MB_ICONWARNING);
        SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_CPT_REGEX));
        return false;
    }
    if (rule.rule == ChapterRule::Rule::REGEX)
    {
        try
        {
            std::wregex e(rule.regex);
        }
        catch (...)
        {
            messageBox(IDS_INVALID_REGEX, IDS_WARN, MB_OK | MB_ICONWARNING);
            SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_CPT_REGEX));
            return false;
        }
    }
    return true;
}

bool AdvanceSettingDlg::isRuleChanged(const ChapterRule& rule)
{
    auto& chapterRule = Settings::instance().data().chapterRule;
    if (chapterRule.rule != rule.rule)
    {
        return true;
    }
    if (rule.rule == ChapterRule::Rule::KEYWORD)
    {
        return chapterRule.keyword != rule.keyword;
    }
    if (rule.rule == ChapterRule::Rule::REGEX)
    {
        return chapterRule.regex != rule.regex;
    }
    return false;
}

LRESULT AdvanceSettingDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
        case IDC_RADIO_CPT_DEFAULT:
            switchRule(ChapterRule::Rule::DEFAULT);
            break;
        case IDC_RADIO_CPT_KEYWORD:
            switchRule(ChapterRule::Rule::KEYWORD);
            break;
        case IDC_RADIO_CPT_REGEX:
            switchRule(ChapterRule::Rule::REGEX);
            break;
        }
        break;

    default:
        break;
    }
    return (INT_PTR)FALSE;
}