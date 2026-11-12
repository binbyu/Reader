#include "DpiAwareness.h"
#include "common.h"

DpiAwareness& DpiAwareness::instance()
{
    static DpiAwareness instance;
    return instance;
}

UINT DpiAwareness::dpi() const
{
    return m_dpix;
}

UINT DpiAwareness::windowDpi(HWND hWnd) const
{
    typedef UINT (WINAPI *GetDpiForWindowFunc)(HWND);
    static GetDpiForWindowFunc getDpiForWindow = reinterpret_cast<GetDpiForWindowFunc>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    UINT windowDpi = getDpiForWindow ? getDpiForWindow(hWnd) : 0;
    return windowDpi ? windowDpi : dpi();
}

float DpiAwareness::scaleFactor() const
{
    return (float)m_dpix / DEFAULT_DPI;
}

void DpiAwareness::setDpi(UINT dpiX, UINT dpiY)
{
    m_dpix = dpiX;
    m_dpiy = dpiY;
}

BOOL DpiAwareness::adjustWindowRectExForDpi(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
    typedef BOOL (WINAPI *AdjustWindowRectExForDpiFunc)(LPRECT, DWORD, BOOL, DWORD, UINT);
    static AdjustWindowRectExForDpiFunc adjustWindowRectExForDpi = (AdjustWindowRectExForDpiFunc)(GetProcAddress(GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi"));

    if (adjustWindowRectExForDpi) return adjustWindowRectExForDpi(lpRect, dwStyle, bMenu, dwExStyle, m_dpix);
    else return AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
}

int DpiAwareness::getSystemMetricsForDpi(int nIndex)
{
    typedef int (WINAPI *GetSystemMetricsForDpiFunc)(int, UINT);
    static GetSystemMetricsForDpiFunc getSystemMetricsForDpi = (GetSystemMetricsForDpiFunc)(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetSystemMetricsForDpi"));

    if (getSystemMetricsForDpi) return getSystemMetricsForDpi(nIndex, m_dpix);
    else return GetSystemMetrics(nIndex);
}

BOOL DpiAwareness::systemParametersInfoForDpi(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
    typedef BOOL (WINAPI *SystemParametersInfoForDpiFunc)(UINT, UINT, PVOID, UINT, UINT);
    static SystemParametersInfoForDpiFunc systemParametersInfoForDpi = (SystemParametersInfoForDpiFunc)(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SystemParametersInfoForDpi"));

    if (systemParametersInfoForDpi) return systemParametersInfoForDpi(uiAction, uiParam, pvParam, fWinIni, m_dpix);
    else return SystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}

void DpiAwareness::toPhysical(int& v) const
{
    v = MulDiv(v, m_dpix, DEFAULT_DPI);
}

void DpiAwareness::toLogical(int& v) const
{
    v = MulDiv(v, DEFAULT_DPI, m_dpix);
}

void DpiAwareness::toPhysical(LONG& v) const
{
    v = MulDiv(v, m_dpix, DEFAULT_DPI);
}

void DpiAwareness::toLogical(LONG& v) const
{
    v = MulDiv(v, DEFAULT_DPI, m_dpix);
}

void DpiAwareness::toPhysical(POINT& v) const
{
    toPhysical(v.x);
    toPhysical(v.y);
}

void DpiAwareness::toLogical(POINT& v) const
{
    toLogical(v.x);
    toLogical(v.y);
}

void DpiAwareness::toPhysical(SIZE& v) const
{
    toPhysical(v.cx);
    toPhysical(v.cy);
}

void DpiAwareness::toLogical(SIZE& v) const
{
    toLogical(v.cx);
    toLogical(v.cy);
}

void DpiAwareness::toPhysical(RECT& v) const
{
    POINT lt = { v.left, v.top };
    POINT rb = { v.right, v.bottom };
    toPhysical(lt);
    toPhysical(rb);
    v.left = lt.x;
    v.top = lt.y;
    v.right = rb.x;
    v.bottom = rb.y;
}

void DpiAwareness::toLogical(RECT& v) const
{
    POINT lt = { v.left, v.top };
    POINT rb = { v.right, v.bottom };
    toLogical(lt);
    toLogical(rb);
    v.left = lt.x;
    v.top = lt.y;
    v.right = rb.x;
    v.bottom = rb.y;
}

void DpiAwareness::toPhysical(LOGFONT& v) const
{
    toPhysical(v.lfWidth);
    toPhysical(v.lfHeight);
}

void DpiAwareness::toLogical(LOGFONT& v) const
{
    toLogical(v.lfWidth);
    toLogical(v.lfHeight);
}

void DpiAwareness::toPhysical(WINDOWPLACEMENT& v) const
{
    toPhysical(v.ptMinPosition);
    toPhysical(v.ptMaxPosition);
    toPhysical(v.rcNormalPosition);
}

void DpiAwareness::toLogical(WINDOWPLACEMENT& v) const
{
    toLogical(v.ptMinPosition);
    toLogical(v.ptMaxPosition);
    toLogical(v.rcNormalPosition);
}

void DpiAwareness::toPhysical(LayoutConfig& v) const
{
    toPhysical(v.borderL);
    toPhysical(v.borderT);
    toPhysical(v.borderR);
    toPhysical(v.borderB);
    toPhysical(v.charGap);
    toPhysical(v.lineGap);
    toPhysical(v.paragraphGap);
}

void DpiAwareness::toLogical(LayoutConfig& v) const
{
    toLogical(v.borderL);
    toLogical(v.borderT);
    toLogical(v.borderR);
    toLogical(v.borderB);
    toLogical(v.charGap);
    toLogical(v.lineGap);
    toLogical(v.paragraphGap);
}

DpiAwareness::DpiAwareness()
{
    HDC hdc = GetDC(NULL);
    m_dpix = hdc ? GetDeviceCaps(hdc, LOGPIXELSX) : DEFAULT_DPI;
    m_dpiy = hdc ? GetDeviceCaps(hdc, LOGPIXELSY) : DEFAULT_DPI;
    if (hdc) ReleaseDC(NULL, hdc);
    if (!m_dpix) m_dpix = DEFAULT_DPI;
    if (!m_dpiy) m_dpiy = DEFAULT_DPI;
}

DpiAwareness::~DpiAwareness()
{
}