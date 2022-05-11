#include "resource.h"
#include "Advset.h"
#include "Editctrl.h"
#include "Book.h"
#include <regex>


static chapter_rule_t *g_rule = NULL;
static BOOL g_bChanged = FALSE;
static INT_PTR CALLBACK ADV_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern Book *_Book;
extern LRESULT OnOpenItem(HWND hWnd, int item_id, BOOL forced);
extern int MessageBox_(HWND, UINT, UINT, UINT);

void ADV_OpenDlg(HINSTANCE hInst, HWND hWnd, chapter_rule_t *rule)
{
    g_rule = rule;
    g_bChanged = FALSE;
    if (EC_IsEditMode())
    {
        EC_LeaveEditMode();
    }
    DialogBox(hInst, MAKEINTRESOURCE(IDD_ADVSET), hWnd, ADV_DlgProc);
    if (g_bChanged)
    {
        // reload book
        if (_Book && _Book->GetBookType() == book_text)
            OnOpenItem(hWnd, 0, TRUE);
    }
}

static INT_PTR CALLBACK ADV_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int s_rule = 0;

    switch (message)
    {
    case WM_INITDIALOG:
        s_rule = g_rule->rule;
        if (s_rule == 0) // default
        {
            SendMessage(GetDlgItem(hDlg, IDC_RADIO_CPT_DEFAULT), BM_SETCHECK, BST_CHECKED, NULL);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_REGEX), FALSE);
        }
        else if (s_rule == 1) // keyword
        {
            SendMessage(GetDlgItem(hDlg, IDC_RADIO_CPT_KEYWORD), BM_SETCHECK, BST_CHECKED, NULL);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_REGEX), FALSE);
        }
        else if (s_rule == 2) // regex
        {
            SendMessage(GetDlgItem(hDlg, IDC_RADIO_CPT_REGEX), BM_SETCHECK, BST_CHECKED, NULL);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_REGEX), TRUE);
        }
        SetDlgItemText(hDlg, IDC_EDIT_CPT_KEYWORD, g_rule->keyword);
        SetDlgItemText(hDlg, IDC_EDIT_CPT_REGEX, g_rule->regex);
        SetFocus(GetDlgItem(hDlg, IDOK));
        return (INT_PTR)FALSE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                TCHAR temp[256] = {0};
                if (s_rule == 1)
                {
                    GetDlgItemText(hDlg, IDC_EDIT_CPT_KEYWORD, temp, 255);
                    if (!temp[0])
                    {
                        MessageBox_(hDlg, IDS_EMPTY_KEYWORD, IDS_ERROR, MB_OK|MB_ICONWARNING);
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD));
                        return (INT_PTR)FALSE;
                    }
                    else
                    {
                        if (wcscmp(g_rule->keyword, temp) != 0)
                        {
                            g_bChanged = TRUE;
                        }
                    }
                }
                else if (s_rule == 2)
                {
                    GetDlgItemText(hDlg, IDC_EDIT_CPT_REGEX, temp, 255);
                    if (!temp[0])
                    {
                        MessageBox_(hDlg, IDS_EMPTY_REGEX, IDS_ERROR, MB_OK|MB_ICONWARNING);
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT_CPT_REGEX));
                        return (INT_PTR)FALSE;
                    }
                    else
                    {
                        // check regex
                        try
                        {
                            std::wregex e(temp);
                        }
                        catch (...)
                        {
                            MessageBox_(hDlg, IDS_INVALID_REGEX, IDS_ERROR, MB_OK|MB_ICONWARNING);
                            SetFocus(GetDlgItem(hDlg, IDC_EDIT_CPT_REGEX));
                            return (INT_PTR)FALSE;
                        }

                        if (wcscmp(g_rule->regex, temp) != 0)
                        {
                            g_bChanged = TRUE;
                        }
                    }
                }

                GetDlgItemText(hDlg, IDC_EDIT_CPT_KEYWORD, g_rule->keyword, 255);
                GetDlgItemText(hDlg, IDC_EDIT_CPT_REGEX, g_rule->regex, 255);
                if (s_rule != g_rule->rule)
                {
                    g_rule->rule = s_rule;
                    g_bChanged = TRUE;
                }
                if (g_bChanged)
                {   
                    // call book to update chapter
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;
                }
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)FALSE;
            }
            break;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)FALSE;
            break;
        case IDC_RADIO_CPT_DEFAULT:
            s_rule = 0;
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_REGEX), FALSE);
            break;
        case IDC_RADIO_CPT_KEYWORD:
            s_rule = 1;
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_REGEX), FALSE);
            break;
        case IDC_RADIO_CPT_REGEX:
            s_rule = 2;
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_KEYWORD), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CPT_REGEX), TRUE);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}
