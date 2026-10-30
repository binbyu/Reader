#pragma once
#ifdef ENABLE_NETWORK
#include "Dialog.h"
#include "ListView.h"
#include "Settings.h"
#include "https.h"

class BookSourceMgrDlg : public Dialog
{
public:
    BookSourceMgrDlg(HINSTANCE hInstance, HWND hParent);
    ~BookSourceMgrDlg();

    void setCurrentBookSourceIndex(int index);
    int  currentBookSourceIndex() const;

protected:
    LRESULT onInitDialog();
    LRESULT onCloseButtonClick();
    LRESULT onExportButtonClick();
    LRESULT onImportButtonClick();
    LRESULT onManualButtonClick();
    LRESULT onSyncButtonClick();
    LRESULT onSaveButtonClick();
    LRESULT onAddButtonClick();
    LRESULT onListSelected(int index, BOOL selected);
    LRESULT onListMoveUp(int index);
    LRESULT onListMoveDown(int index);
    LRESULT onListMoveTop(int index);
    LRESULT onListMoveBottom(int index);
    LRESULT onListDelete(int index);
    LRESULT onListClear();
    LRESULT onComboBoxChanged();
    LRESULT onReqCompleter(WPARAM wParam, LPARAM lParam);

protected:
    void setDataToUI(const BookSource& data);
    void getDataFromUI(BookSource& data);
    void clearUI();
    void updateBookSourceList(int index);
    void enableControls(bool enable);
    int  mergeBookSourceList(const std::vector<BookSource>& sources);
    bool isBookSourceEmpty(const BookSource& source);
    bool isBookSourceValid(const BookSource& source);
    bool isBookSourceExist(const BookSource& source, int exceptIndex);
    bool isBookSourceModified(const BookSource& source);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
    int m_currentIndex;
    request_handle_t m_hSyncReq;
    ListView* m_list;
};
#endif