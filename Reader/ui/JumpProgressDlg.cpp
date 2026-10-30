#include "JumpProgressDlg.h"
#include "resource.h"

JumpProgressDlg::JumpProgressDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_JUMPPROGRESS)
    , m_progress(0.0f)
{
}

JumpProgressDlg::~JumpProgressDlg()
{
}

LRESULT JumpProgressDlg::onInitDialog()
{
    centerParent();
    updateProgress();
    return FALSE;
}

void JumpProgressDlg::updateProgress()
{
    wchar_t buffer[16] = { 0 };
    _swprintf(buffer, L"%0.2f", m_progress);

    HWND hEdit = GetDlgItem(m_hWnd, IDC_EDIT_JP);
    SetWindowText(hEdit, buffer);
    SetFocus(hEdit);
    SendMessage(hEdit, EM_SETSEL, 0, -1);
}

void JumpProgressDlg::setProgress(float progress)
{
    m_progress = progress;
}

float JumpProgressDlg::progress() const
{
    return m_progress;
}

LRESULT JumpProgressDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
        case IDC_EDIT_JP:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                HWND hEdit = GetDlgItem(m_hWnd, IDC_EDIT_JP);
                wchar_t buffer[16] = { 0 };
                GetWindowText(hEdit, buffer, 16);

                m_progress = (float)_wtof(buffer);
                if (m_progress < 0.0f)
                {
                    m_progress = 0.0f;
                    updateProgress();
                }
                if (m_progress > 100.0f)
                {
                    m_progress = 100.0f;
                    updateProgress();
                }
            }
        }
        break;

    default:
        break;
    }
    return (INT_PTR)FALSE;
}