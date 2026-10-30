#pragma once
#include "Dialog.h"
#include "IBook.h"

class AdvanceSettingDlg : public Dialog
{
public:
    AdvanceSettingDlg(HINSTANCE hInstance, HWND hParent);
    ~AdvanceSettingDlg();

    bool isChanged() const;

protected:
    LRESULT onInitDialog();
    LRESULT onEndDialog(INT_PTR nResult);
    
protected:
    void switchRule(ChapterRule::Rule rule);
    bool isValidRule(const ChapterRule& rule);
    bool isRuleChanged(const ChapterRule& rule);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
    bool m_bChanged;
};