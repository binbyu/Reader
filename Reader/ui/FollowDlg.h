#pragma once
#include "Dialog.h"
#include "IRenderer.h"

class FollowDlg : public Dialog
{
public:
    FollowDlg(HINSTANCE hInstance, HWND hParent);
    ~FollowDlg();

protected:
    LRESULT onInitDialog();
    LRESULT onPaint();
    LRESULT onLButtonDown(WPARAM wParam, LPARAM lParam);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

protected:
    Image* m_image;
};