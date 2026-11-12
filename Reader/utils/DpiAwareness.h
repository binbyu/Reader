#pragma once
#include "framework.h"
#include "types.h"

class DpiAwareness
{
public:
    static DpiAwareness& instance();
    
    UINT  dpi() const;
    UINT  windowDpi(HWND hWnd) const;
    float scaleFactor() const;
    void  setDpi(UINT dpiX, UINT dpiY);
    BOOL  adjustWindowRectExForDpi(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
    int   getSystemMetricsForDpi(int nIndex);
    BOOL  systemParametersInfoForDpi( UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);

    void toPhysical(int& v) const;
    void toLogical(int& v) const;
    void toPhysical(LONG& v) const;
    void toLogical(LONG& v) const;
    void toPhysical(POINT& v) const;
    void toLogical(POINT& v) const;
    void toPhysical(SIZE& v) const;
    void toLogical(SIZE& v) const;
    void toPhysical(RECT& v) const;
    void toLogical(RECT& v) const;
    void toPhysical(LOGFONT& v) const;
    void toLogical(LOGFONT& v) const;
    void toPhysical(WINDOWPLACEMENT& v) const;
    void toLogical(WINDOWPLACEMENT& v) const;
    void toPhysical(LayoutConfig& v) const;
    void toLogical(LayoutConfig& v) const;

private:
    DpiAwareness();
    ~DpiAwareness();

private:
    UINT m_dpix;
    UINT m_dpiy;
};