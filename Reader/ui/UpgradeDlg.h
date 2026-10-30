#pragma once
#ifdef ENABLE_NETWORK
#include "Dialog.h"
#include "Settings.h"

class UpgradeDlg : public Dialog
{
public:
    UpgradeDlg(HINSTANCE hInstance, HWND hParent);
    ~UpgradeDlg();

    static void checkVersion(HWND hParent);
    void setVersion(VersionInfo* info);

protected:
    LRESULT onInitDialog();
    LRESULT onIgnoreButtonClick();
    LRESULT onDownButtonClick();

protected:
    static int vercmp(const wchar_t* v1, const wchar_t* v2);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

protected:
    VersionInfo* m_info;
};
#endif