#include "RewardDlg.h"
#include "resource.h"
#include "barcode.h"
#include "DpiAwareness.h"

RewardDlg::RewardDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_REWARD)
    , m_imageL(nullptr)
    , m_imageR(nullptr)
    , m_rectL({ 0 })
    , m_rectR({ 0 })
{
}

RewardDlg::~RewardDlg()
{
    if (m_imageL)
    {
        delete m_imageL;
        m_imageL = nullptr;
    }
    if (m_imageR)
    {
        delete m_imageR;
        m_imageR = nullptr;
    }
}

LRESULT RewardDlg::onInitDialog()
{
    m_imageL = IRenderer::instance()->loadImageFromStream(IMG::barcodeWechat(), IMG::BARCODE_WECHAT_LENGHT);
    m_imageR = IRenderer::instance()->loadImageFromStream(IMG::barcodeAlipay(), IMG::BARCODE_ALIPAY_LENGHT);
    float scale = DpiAwareness::instance().scaleFactor();

    if (m_imageL)
    {
        if (scale <= 1.0f)
        {
            SetRect(&m_rectL, 0, 0, (int)(m_imageL->width() / 2.0f), (int)(m_imageL->height() / 2.0f));
        }
        else if (scale <= 1.25f)
        {
            SetRect(&m_rectL, 0, 0, (int)(m_imageL->width() / 3.0f * 2.0f), (int)(m_imageL->height() / 3.0f * 2.0f));
        }
        else
        {
            SetRect(&m_rectL, 0, 0, m_imageL->width(), m_imageL->height());
        }

        if (m_imageR)
        {
            m_rectR.top = m_rectL.top;
            m_rectR.bottom = m_rectL.bottom;
            m_rectR.left = m_rectL.right + 2;
            m_rectR.right = (m_rectR.bottom - m_rectR.top) * m_imageR->width() / m_imageR->height() + m_rectR.left;
        }
    }

    RECT rect;
    rect.top = m_rectL.top;
    rect.bottom = m_rectL.bottom;
    rect.left = m_rectL.left;
    rect.right = m_rectR.right;

    AdjustWindowRect(&rect, (DWORD)GetWindowLongPtr(m_hWnd, GWL_STYLE), FALSE);
    centerParent(rect);
    return TRUE;
}

LRESULT RewardDlg::onPaint()
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;

    hdc = BeginPaint(m_hWnd, &ps);

    GetClientRect(m_hWnd, &rect);
    IRenderer::instance()->beginPaint(m_hWnd, hdc, rect, {});
    if (m_imageL)
    {
        IRenderer::instance()->drawImage(m_imageL, m_rectL);
    }
    if (m_imageR)
    {
        IRenderer::instance()->drawImage(m_imageR, m_rectR);
    }
    IRenderer::instance()->endPaint();

    return EndPaint(m_hWnd, &ps);
}

LRESULT RewardDlg::onLButtonDown(WPARAM wParam, LPARAM lParam)
{
    return EndDialog(m_hWnd, LOWORD(wParam));
}


LRESULT RewardDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
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