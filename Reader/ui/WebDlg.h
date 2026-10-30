/**
 * @file WebDlg.h
 * @brief Provides a LAN Web server to sync and read books with the Legado mobile app.
 *
 * Original Author: NanoKimi (https://github.com/NanoKimi)
 * Current  Author: binbyu   (https://github.com/binbyu)
 */

#pragma once
#ifdef ENABLE_NETWORK
#include "Dialog.h"
#include "ListView.h"
#include "Settings.h"
#include "https.h"

struct WebShelfResult
{
    HWND hDlg;
    int  error;     // 0: succ, 1: fail
    int  total;
    int  status;    // http status
    int  bodylen;
    std::vector<WebShelfItem>* items;
};

class WebDlg : public Dialog
{
public:
    WebDlg(HINSTANCE hInstance, HWND hParent);
    ~WebDlg();

    std::wstring openFileName() const;

protected:
    LRESULT onInitDialog();
    LRESULT onOpenButtonClick();
    LRESULT onCloseButtonClick();
    LRESULT onGoButtonClick();
    LRESULT onRefreshButtonClick();
    LRESULT onSyncButtonClick();
    LRESULT onReqCompleter(WPARAM wParam, LPARAM lParam);
    LRESULT onSize(int width, int height);

protected:
    void initListView();
    void refreshListView();
    void initServerUrl();
    void enableControls(BOOL enable);
    bool isMatched(const std::wstring& filter, const WebShelfItem& item);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
    ListView* m_list;
    WebShelfList m_data;
    request_handle_t m_hRequest;
    std::wstring m_fileName;
};
#endif