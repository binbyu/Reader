#ifndef __EDIT_CTRL_H__
#define __EDIT_CTRL_H__

#include "types.h"

void EC_EnterEditMode(HINSTANCE hInst, HWND hWnd, LOGFONT *font, TCHAR *text);
void EC_UpdateEditMode(HWND hWnd, TCHAR *text);
void EC_LeaveEditMode(void);
BOOL EC_IsEditMode(void);

#endif
