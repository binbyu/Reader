#pragma once
#include "IWnd.h"
#include "Settings.h"

class MainWin;
class EditController: public IWnd
{
public:
    EditController(MainWin* mainWin);
    ~EditController();

    bool create();
    void beginEdit();
    void endEdit();
    bool isEditing() const;
    void updateLayout();

protected:
    void initOriginalText();
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK subclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

private:
    MainWin*     m_mainWin;
    bool         m_isEditing;
    DisplayMode  m_originalMode;
    int          m_bookStart;
    int          m_originalLen;
    std::wstring m_originalEditText;
    HFONT        m_hFont;
};