#pragma once
#include "framework.h"

class DpiAwareness
{
public:
    static DpiAwareness& instance();
    
    UINT  dpi() const;
    float scaleFactor() const;
    void  setDpi(UINT dpiX, UINT dpiY);

    void logicalToPhysical(int src, int& dst) const;
    void physicalToLogical(int src, int& dst) const;
    void logicalToPhysical(const POINT& src, POINT& dst) const;
    void physicalToLogical(const POINT& src, POINT& dst) const;
    void logicalToPhysical(const SIZE& src, SIZE& dst) const;
    void physicalToLogical(const SIZE& src, SIZE& dst) const;
    void logicalToPhysical(const RECT& src, RECT& dst) const;
    void physicalToLogical(const RECT& src, RECT& dst) const;
    void logicalToPhysical(const LOGFONT& src, LOGFONT& dst) const;
    void physicalToLogical(const LOGFONT& src, LOGFONT& dst) const;
    void logicalToPhysical(const WINDOWPLACEMENT& src, WINDOWPLACEMENT& dst) const;
    void physicalToLogical(const WINDOWPLACEMENT& src, WINDOWPLACEMENT& dst) const;

private:
    DpiAwareness();
    ~DpiAwareness();

private:
    UINT m_dpix;
    UINT m_dpiy;
};