#pragma once
#ifdef ENABLE_NETWORK
#include "Dialog.h"
#include "Utils.h"

class NetworkSettingDlg : public Dialog
{
public:
    NetworkSettingDlg(HINSTANCE hInstance, HWND hParent);
    ~NetworkSettingDlg();

    bool isChanged() const;

protected:
    LRESULT onInitDialog();
    LRESULT onSave();
    void enableProxySettings(bool enable);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

protected:
    std::vector<AdapterInfo> m_adapters;
    bool m_changed;
};
#endif