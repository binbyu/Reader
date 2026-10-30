#include "FollowDlg.h"
#include "resource.h"
#include "barcode.h"
#include "DpiAwareness.h"

FollowDlg::FollowDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_REWARD)
    , m_image(nullptr)
{
}

FollowDlg::~FollowDlg()
{
    if (m_image)
    {
        delete m_image;
        m_image = nullptr;
    }
}

LRESULT FollowDlg::onInitDialog()
{
    m_image = IRenderer::instance()->loadImageFromStream(IMG::barcodeMp(), IMG::BARCODE_MP_LENGHT);
    if (m_image)
    {
        float scale = DpiAwareness::instance().scaleFactor();
        RECT rect;
        SetRect(&rect, 0, 0, (int)(m_image->width() * scale), (int)(m_image->height() * scale));
        AdjustWindowRect(&rect, (DWORD)GetWindowLongPtr(m_hWnd, GWL_STYLE), FALSE);
        centerParent(rect);
    }
    return TRUE;
}

LRESULT FollowDlg::onPaint()
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rc;

    hdc = BeginPaint(m_hWnd, &ps);

    if (m_image)
    {
        GetClientRect(m_hWnd, &rc);
        IRenderer::instance()->beginPaint(m_hWnd, hdc, rc, {});
        IRenderer::instance()->drawImage(m_image, rc);
        IRenderer::instance()->endPaint();
    }

    return EndPaint(m_hWnd, &ps);
}

LRESULT FollowDlg::onLButtonDown(WPARAM wParam, LPARAM lParam)
{
    return EndDialog(m_hWnd, LOWORD(wParam));
}


LRESULT FollowDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
        }
        break;

    case WM_PAINT:
        return onPaint();

    case WM_LBUTTONDOWN:
        return onLButtonDown(wParam, lParam);

    default:
        break;
    }
    return (INT_PTR)FALSE;
}