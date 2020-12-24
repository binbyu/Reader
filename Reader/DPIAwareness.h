#ifndef __DPI_AWARENESS_H__
#define __DPI_AWARENESS_H__

int GetCxScreenForDpi(void);
int GetCyScreenForDpi(void);

void UpdateLayoutForDpi(RECT *rect);
void UpdateFontForDpi(LOGFONT *lf);

void RestoreRectForDpi(RECT *rect);
void RestoreFontForDpi(LOGFONT *lf);

void DpiChanged(HWND hWnd, LOGFONT *lf, RECT *rect, WPARAM newdpi, RECT *newRect);

double GetDpiScaled(void);


#endif
