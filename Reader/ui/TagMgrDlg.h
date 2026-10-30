#pragma once
#if ENABLE_TAG
#include "Dialog.h"
#include "ListView.h"

class TagMgrDlg : public Dialog
{
public:
    TagMgrDlg(HINSTANCE hInstance, HWND hParent);
    ~TagMgrDlg();

protected:
    LRESULT onInitDialog();
    LRESULT onCloseButtonClick(WPARAM wParam);
    LRESULT onAddButtonClick();
    LRESULT onListSelected(int index, BOOL selected);
    LRESULT onListChecked(int index, BOOL checked);
    LRESULT onListMoveUp(int index);
    LRESULT onListMoveDown(int index);
    LRESULT onListMoveTop(int index);
    LRESULT onListMoveBottom(int index);
    LRESULT onListDelete(int index);
    LRESULT onListClear();
    LRESULT onListEdit(int index);
    LRESULT onUpdatePreview(HDC hdc, RECT rect);

protected:
    void updateTagList(int index);
    void updatePreview();

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

protected:
    ListView* m_list;
};
#endif