#pragma once
#include "Dialog.h"
#include "Settings.h"

class KeySettingDlg : public Dialog
{
public:
    KeySettingDlg(HINSTANCE hInstance, HWND hParent);
    ~KeySettingDlg();

protected:
    LRESULT onInitDialog();
    LRESULT onRestoreDefault();
    LRESULT onSave();
    LRESULT onExit();

protected:
    void initData(KeysetInfo& keyset);
    void loadData(KeysetInfo& keyset);
    bool isExtendedKey(BYTE vk);
    bool isValid(KeysetInfo& keyset);
    DWORD hotToMod(DWORD value);
    DWORD modToHot(DWORD value);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK hotKeyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

protected:
};