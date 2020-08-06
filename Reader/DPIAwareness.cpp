#include "stdafx.h"
#include "DPIAwareness.h"

#define DEFAULT_DPI             96

static int g_lastDpix = DEFAULT_DPI;
static int g_lastDpiy = DEFAULT_DPI;

static void GetDPI(HWND hWnd, int *dpix, int *dpiy)
{
    HDC hdc;

    hdc = GetDC(hWnd);
    *dpix = GetDeviceCaps(hdc, LOGPIXELSX);
    *dpiy = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hWnd, hdc);
    g_lastDpix = *dpix;
    g_lastDpiy = *dpiy;
}

void UpdateLayoutForDpi(HWND hWnd, const RECT *rect, BYTE *isDefault)
{
    int dpix = 0, dpiy = 0;
    int scaledx, scaledy, scaledw, scaledh;

    GetDPI(hWnd, &dpix, &dpiy);

    if (dpix == DEFAULT_DPI && dpiy == DEFAULT_DPI)
    {
        return;
    }

    if (*isDefault)
    {
        scaledw = MulDiv(rect->right-rect->left, dpix, DEFAULT_DPI);
        scaledh = MulDiv(rect->bottom-rect->top, dpiy, DEFAULT_DPI);
        scaledx = (GetSystemMetrics(SM_CXSCREEN) - scaledw) / 2;
        scaledy = (GetSystemMetrics(SM_CYSCREEN) - scaledh) / 2;
        *isDefault = 0;
    }
    else
    {
        scaledx = MulDiv(rect->left, dpix, DEFAULT_DPI);
        scaledy = MulDiv(rect->top, dpiy, DEFAULT_DPI);
        scaledw = MulDiv(rect->right-rect->left, dpix, DEFAULT_DPI);
        scaledh = MulDiv(rect->bottom-rect->top, dpiy, DEFAULT_DPI);
    }
    

    SetWindowPos(hWnd, hWnd, scaledx, scaledy, scaledw, scaledh, SWP_NOZORDER | SWP_NOACTIVATE);
}

void UpdateFontForDpi(HWND hWnd, LOGFONT *lf)
{
    int dpix, dpiy;

    GetDPI(hWnd, &dpix, &dpiy);
    lf->lfWidth = MulDiv(lf->lfWidth, dpix, DEFAULT_DPI);
    lf->lfHeight = MulDiv(lf->lfHeight, dpiy, DEFAULT_DPI);
}

void RestoreRectForDpi(HWND hWnd, RECT *rect)
{
    int dpix = 0, dpiy = 0;
    int scaledx, scaledy, scaledw, scaledh;

    GetDPI(hWnd, &dpix, &dpiy);

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

void RestoreFontForDpi(HWND hWnd, LOGFONT *lf)
{
    int dpix, dpiy;

    GetDPI(hWnd, &dpix, &dpiy);
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

    memcpy(rect, newRect, sizeof(RECT));
    SetWindowPos(hWnd, hWnd, newRect->left, newRect->top, newRect->right-newRect->left, newRect->bottom-newRect->top, SWP_NOZORDER | SWP_NOACTIVATE);
}
