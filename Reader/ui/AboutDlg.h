#pragma once
#include "Dialog.h"

class AboutDlg : public Dialog
{
public:
    AboutDlg(HINSTANCE hInstance, HWND hParent);
    ~AboutDlg();

protected:
    LRESULT onInitDialog();
    LRESULT onReward();
    LRESULT onFollow();
    LRESULT onLButtonDown(WPARAM wParam, LPARAM lParam);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
};