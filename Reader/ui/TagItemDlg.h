#pragma once
#if ENABLE_TAG
#include "Dialog.h"
#include "Settings.h"

class TagItemDlg : public Dialog
{
public:
    TagItemDlg(HINSTANCE hInstance, HWND hParent);
    ~TagItemDlg();

    void SetTagIndex(int index);
    const TagInfo& GetTagItem() const;

protected:
    LRESULT onInitDialog();
    LRESULT onCloseButtonClick(WPARAM wParam);
    LRESULT onSaveButtonClick();
    LRESULT onTextFontButtonClick();
    LRESULT onBGColorButtonClick();
    LRESULT onKeywordChanged();
    LRESULT onUpdatePreview(HDC hdc, RECT rect);

protected:
    void updatePreview();
    bool isExist();
    bool isValid();

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

protected:
    int m_index;
    TagInfo m_item;
};
#endif