#pragma once
#ifdef ENABLE_NETWORK
#include "Dialog.h"
#include "ListView.h"
#include "https.h"
#include <regex>

class SimpleSearchDlg : public Dialog
{
private:
    struct ChapterItem
    {
        std::string url;
        std::wstring title;
        BOOL checked;
    };
    enum State
    {
        IDLE,
        PARSING,
        PREVIEW,
    };
public:
    SimpleSearchDlg(HINSTANCE hInstance, HWND hParent);
    ~SimpleSearchDlg();

    std::wstring openFileName() const;

protected:
    LRESULT onInitDialog();
    LRESULT onCloseButtonClick();
    LRESULT onParseButtonClick();
    LRESULT onFilterButtonClick();
    LRESULT onOpenBookButtonClick();
    LRESULT onSize(int width, int height);
    LRESULT onReqCompleter(WPARAM wParam, LPARAM lParam);
    LRESULT onListChecked(int index, BOOL checked);
    LRESULT onListPreview(int index);
    LRESULT onListMoveUp(int index);
    LRESULT onListMoveDown(int index);
    LRESULT onListMoveTop(int index);
    LRESULT onListMoveBottom(int index);

protected:
    void buildListView(int index);
    void enableControls();
    bool isChapter(const std::wstring& text);
    bool isExist(const std::string& url, const std::wstring& text);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    static unsigned int completer(request_result_t* result);

protected:
    ListView* m_list;
    std::wstring m_fileName;
    request_handle_t m_hRequest;
    std::vector<ChapterItem> m_chapters;
    std::string m_url;
    std::wstring m_content;
    State m_state;
    std::wregex* m_filter;
};
#endif
