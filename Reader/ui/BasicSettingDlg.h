#pragma once
#include "Dialog.h"
#include "Settings.h"

class BasicSettingDlg : public Dialog
{
public:
    BasicSettingDlg(HINSTANCE hInstance, HWND hParent);
    ~BasicSettingDlg();

    bool isMinToTrayChanged() const;
    bool isHideTaskbarChanged() const;

protected:
    LRESULT onInitDialog();
    LRESULT onEndDialog(INT_PTR nResult);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
    bool m_isMinToTrayChanged;
    bool m_isHideTaskbarChanged;
};