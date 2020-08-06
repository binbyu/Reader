#ifndef __DPI_AWARENESS_H__
#define __DPI_AWARENESS_H__

void UpdateLayoutForDpi(HWND hWnd, const RECT *rect, BYTE *isDefault);
void UpdateFontForDpi(HWND hWnd, LOGFONT *lf);

void RestoreRectForDpi(HWND hWnd, RECT *rect);
void RestoreFontForDpi(HWND hWnd, LOGFONT *lf);

void DpiChanged(HWND hWnd, LOGFONT *lf, RECT *rect, WPARAM newdpi, RECT *newRect);


#endif
