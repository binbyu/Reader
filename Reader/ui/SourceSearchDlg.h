#pragma once
#ifdef ENABLE_NETWORK
#include "Dialog.h"
#include "ListView.h"
#include "https.h"
#include "Settings.h"
#include "RequestQueue.h"
#include <list>

class SourceSearchDlg : public Dialog
{
private:
    struct RData
    {
        RData(HWND hWnd, std::wstring keyword, int bsIndex, bool isGlobal) : hWnd(hWnd), keyword(keyword), bsIndex(bsIndex), isGlobal(isGlobal) {}
        ~RData() {}
        HWND hWnd;
        std::wstring keyword;
        int bsIndex;
        bool isGlobal;
    };
    struct PostData
    {
        PostData(bool isGlobal) : isGlobal(isGlobal), errCode(0), html(nullptr) {}
        bool isGlobal;
        int errCode;
        int statusCode;
        int bsIndex;
        int htmlLen;
        char* html;
    };
    struct BookItem
    {
        std::string  bsHost; // identifier of book source
        std::wstring bsName;
        std::wstring bookName;
        std::wstring bookAuthor;
        std::string  bookMainPageUrl;
    };
public:
    SourceSearchDlg(HINSTANCE hInstance, HWND hParent);
    ~SourceSearchDlg();

    std::wstring openFileName() const;

protected:
    LRESULT onInitDialog();
    LRESULT onCloseButtonClick();
    LRESULT onBookSourceMgrButtonClick();
    LRESULT onSearchButtonClick();
    LRESULT onOpenBookButtonClick();
    LRESULT onSize(int width, int height);
    LRESULT onReqCompleter(WPARAM wParam, LPARAM lParam);

protected:
    void updateBookSourceComboBox(int index);
    void enableControls(bool enable);
    void appendListView(BookItem* items, int count);
    void doRequest(int bsIndex, const std::wstring& keyword, bool isGlobal);
    void buildUrl(int bsIndex, http_charset_t charset, const std::wstring& keyword, Request& req);
    void requestCompleter(request_result_t* result, const Request& r);
    void freePostData(PostData* pd);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

protected:
    ListView* m_list;
    RequestQueue m_reqQue;
    std::vector<BookItem> m_bookItems;
    std::wstring m_fileName;
};
#endif