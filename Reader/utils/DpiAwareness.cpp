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

float DpiAwareness::scaleFactor() const
{
    return (float)m_dpix / DEFAULT_DPI;
}

void DpiAwareness::setDpi(UINT dpiX, UINT dpiY)
{
    m_dpix = dpiX;
    m_dpiy = dpiY;
}

void DpiAwareness::logicalToPhysical(int src, int& dst) const
{
    dst = MulDiv(src, m_dpix, DEFAULT_DPI);
}

void DpiAwareness::physicalToLogical(int src, int& dst) const
{
    dst = MulDiv(src, DEFAULT_DPI, m_dpix);
}

void DpiAwareness::logicalToPhysical(const POINT& src, POINT& dst) const
{
    dst.x = MulDiv(src.x, m_dpix, DEFAULT_DPI);
    dst.y = MulDiv(src.y, m_dpiy, DEFAULT_DPI);
}

void DpiAwareness::physicalToLogical(const POINT& src, POINT& dst) const
{
    dst.x = MulDiv(src.x, DEFAULT_DPI, m_dpix);
    dst.y = MulDiv(src.y, DEFAULT_DPI, m_dpiy);
}

void DpiAwareness::logicalToPhysical(const SIZE& src, SIZE& dst) const
{
    dst.cx = MulDiv(src.cx, m_dpix, DEFAULT_DPI);
    dst.cy = MulDiv(src.cy, m_dpiy, DEFAULT_DPI);
}

void DpiAwareness::physicalToLogical(const SIZE& src, SIZE& dst) const
{
    dst.cx = MulDiv(src.cx, DEFAULT_DPI, m_dpix);
    dst.cy = MulDiv(src.cy, DEFAULT_DPI, m_dpiy);
}

void DpiAwareness::logicalToPhysical(const RECT& src, RECT& dst) const
{
    dst.left   = MulDiv(src.left, m_dpix, DEFAULT_DPI);
    dst.top    = MulDiv(src.top, m_dpiy, DEFAULT_DPI);
    dst.right  = MulDiv(src.right, m_dpix, DEFAULT_DPI);
    dst.bottom = MulDiv(src.bottom, m_dpiy, DEFAULT_DPI);
}

void DpiAwareness::physicalToLogical(const RECT& src, RECT& dst) const
{
    dst.left   = MulDiv(src.left, DEFAULT_DPI, m_dpix);
    dst.top    = MulDiv(src.top, DEFAULT_DPI, m_dpiy);
    dst.right  = MulDiv(src.right, DEFAULT_DPI, m_dpix);
    dst.bottom = MulDiv(src.bottom, DEFAULT_DPI, m_dpiy);
}

void DpiAwareness::logicalToPhysical(const LOGFONT& src, LOGFONT& dst) const
{
    dst.lfWidth  = MulDiv(src.lfWidth, m_dpix, DEFAULT_DPI);
    dst.lfHeight = MulDiv(src.lfHeight, m_dpiy, DEFAULT_DPI);
}

void DpiAwareness::physicalToLogical(const LOGFONT& src, LOGFONT& dst) const
{
    dst.lfWidth  = MulDiv(src.lfWidth, DEFAULT_DPI, m_dpix);
    dst.lfHeight = MulDiv(src.lfHeight, DEFAULT_DPI, m_dpiy);
}

void DpiAwareness::logicalToPhysical(const WINDOWPLACEMENT& src, WINDOWPLACEMENT& dst) const
{
    logicalToPhysical(src.ptMinPosition, dst.ptMinPosition);
    logicalToPhysical(src.ptMaxPosition, dst.ptMaxPosition);
    logicalToPhysical(src.rcNormalPosition, dst.rcNormalPosition); 
}

void DpiAwareness::physicalToLogical(const WINDOWPLACEMENT& src, WINDOWPLACEMENT& dst) const
{
    physicalToLogical(src.ptMinPosition, dst.ptMinPosition);
    physicalToLogical(src.ptMaxPosition, dst.ptMaxPosition);
    physicalToLogical(src.rcNormalPosition, dst.rcNormalPosition); 
}

DpiAwareness::DpiAwareness()
{
    HDC hdc;
    hdc = GetDC(NULL);
    m_dpix = GetDeviceCaps(hdc, LOGPIXELSX);
    m_dpiy = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
}

DpiAwareness::~DpiAwareness()
{
}