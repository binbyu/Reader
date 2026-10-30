#pragma once
#include "Dialog.h"

class JumpProgressDlg : public Dialog
{
public:
    JumpProgressDlg(HINSTANCE hInstance, HWND hParent);
    ~JumpProgressDlg();

    void  setProgress(float progress);
    float progress() const;

protected:
    LRESULT onInitDialog();
    void updateProgress();

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
    float m_progress;
};