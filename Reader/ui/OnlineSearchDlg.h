#pragma once
#ifdef ENABLE_NETWORK
#include "Dialog.h"
#include "SourceSearchDlg.h"
#include "SimpleSearchDlg.h"

class OnlineSearchDlg : public Dialog
{
public:
    OnlineSearchDlg(HINSTANCE hInstance, HWND hParent);
    ~OnlineSearchDlg();

    std::wstring openFileName() const;

protected:
    LRESULT onInitDialog();
    LRESULT onSize(int width, int height);

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
    SourceSearchDlg* m_page1;
    SimpleSearchDlg* m_page2;
    int m_tabIndex;
};
#endif