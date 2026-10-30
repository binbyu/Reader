#pragma once
#include "Dialog.h"
#include "IRenderer.h"

class RewardDlg : public Dialog
{
public:
    RewardDlg(HINSTANCE hInstance, HWND hParent);
    ~RewardDlg();

protected:
    LRESULT onInitDialog();
    LRESULT onPaint();
    LRESULT onLButtonDown(WPARAM wParam, LPARAM lParam);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

protected:
    Image* m_imageL;
    Image* m_imageR;
    RECT m_rectL;
    RECT m_rectR;
};