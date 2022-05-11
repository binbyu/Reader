#include "framework.h"
#include "DPIAwareness.h"

#define DEFAULT_DPI             96

static int g_lastDpix = DEFAULT_DPI;
static int g_lastDpiy = DEFAULT_DPI;

static void GetDPI(int *dpix, int *dpiy)
{
    HDC hdc;

    hdc = GetDC(NULL);
    *dpix = GetDeviceCaps(hdc, LOGPIXELSX);
    *dpiy = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
#if 0 // for test
    *dpix = 120;
    *dpiy = 120;
#endif
    g_lastDpix = *dpix;
    g_lastDpiy = *dpiy;
}

int GetCxScreenForDpi(void)
{
    int dpix = 0, dpiy = 0;
    GetDPI(&dpix, &dpiy);
    return MulDiv(GetSystemMetrics(SM_CXSCREEN), DEFAULT_DPI, dpix);
}

int GetCyScreenForDpi(void)
{
    int dpix = 0, dpiy = 0;
    GetDPI(&dpix, &dpiy);
    return MulDiv(GetSystemMetrics(SM_CYSCREEN), DEFAULT_DPI, dpiy);
}

int GetWidthForDpi(int w)
{
    int dpix = 0, dpiy = 0;

    GetDPI(&dpix, &dpiy);

    if (dpix == DEFAULT_DPI && dpiy == DEFAULT_DPI)
    {
        return w;
    }

    return MulDiv(w, dpix, DEFAULT_DPI);
}

int GetHeightForDpi(int h)
{
    int dpix = 0, dpiy = 0;

    GetDPI(&dpix, &dpiy);

    if (dpix == DEFAULT_DPI && dpiy == DEFAULT_DPI)
    {
        return h;
    }

    return MulDiv(h, dpiy, DEFAULT_DPI);
}

void UpdateRectForDpi(RECT *rect)
{
    int dpix = 0, dpiy = 0;
    int scaledx, scaledy, scaledw, scaledh;

    GetDPI(&dpix, &dpiy);

    if (dpix == DEFAULT_DPI && dpiy == DEFAULT_DPI)
    {
        return;
    }

    scaledx = MulDiv(rect->left, dpix, DEFAULT_DPI);
    scaledy = MulDiv(rect->top, dpiy, DEFAULT_DPI);
    scaledw = MulDiv(rect->right - rect->left, dpix, DEFAULT_DPI);
    scaledh = MulDiv(rect->bottom - rect->top, dpiy, DEFAULT_DPI);
    rect->left = scaledx;
    rect->top = scaledy;
    rect->right = scaledx + scaledw;
    rect->bottom = scaledy + scaledh;
}

void UpdateFontForDpi(LOGFONT *lf)
{
    int dpix, dpiy;

    GetDPI(&dpix, &dpiy);

    if (dpix == DEFAULT_DPI && dpiy == DEFAULT_DPI)
    {
        return;
    }

    lf->lfWidth = MulDiv(lf->lfWidth, dpix, DEFAULT_DPI);
    lf->lfHeight = MulDiv(lf->lfHeight, dpiy, DEFAULT_DPI);
}

void RestoreRectForDpi(RECT *rect)
{
    int dpix = 0, dpiy = 0;
    int scaledx, scaledy, scaledw, scaledh;

    GetDPI(&dpix, &dpiy);

    if (dpix == DEFAULT_DPI && dpiy == DEFAULT_DPI)
    {
        return;
    }

    scaledx = MulDiv(rect->left, DEFAULT_DPI, dpix);
    scaledy = MulDiv(rect->top, DEFAULT_DPI, dpiy);
    scaledw = MulDiv(rect->right-rect->left, DEFAULT_DPI, dpix);
    scaledh = MulDiv(rect->bottom-rect->top, DEFAULT_DPI, dpiy);
    rect->left = scaledx;
    rect->top = scaledy;
    rect->right = scaledx + scaledw;
    rect->bottom = scaledy + scaledh;
}

void RestoreFontForDpi(LOGFONT *lf)
{
    int dpix, dpiy;

    GetDPI(&dpix, &dpiy);

    if (dpix == DEFAULT_DPI && dpiy == DEFAULT_DPI)
    {
        return;
    }

    lf->lfWidth = MulDiv(lf->lfWidth, DEFAULT_DPI, dpix);
    lf->lfHeight = MulDiv(lf->lfHeight, DEFAULT_DPI, dpiy);
}

void DpiChanged(HWND hWnd, LOGFONT *lf, RECT *rect, WPARAM newdpi, RECT *newRect)
{
    int newdpiy = HIWORD(newdpi);
    int newdpix = LOWORD(newdpi);

    // set font
    lf->lfWidth = MulDiv(lf->lfWidth, newdpix, g_lastDpix);
    lf->lfHeight = MulDiv(lf->lfHeight, newdpiy, g_lastDpiy);
    g_lastDpix = newdpix;
    g_lastDpiy = newdpiy;

    memcpy(rect, newRect, sizeof(RECT));
    SetWindowPos(hWnd, hWnd, newRect->left, newRect->top, newRect->right-newRect->left, newRect->bottom-newRect->top, SWP_NOZORDER | SWP_NOACTIVATE);
}

double GetDpiScaled(void)
{
    int dpix = 0, dpiy = 0;
    GetDPI(&dpix, &dpiy);
    return (((double)dpix)/((double)DEFAULT_DPI));
}
