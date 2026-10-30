#include "EditController.h"
#include "resource.h"
#include "MainWin.h"

EditController::EditController(MainWin* mainWin)
    : IWnd(mainWin->hInstance(), mainWin->getHWND())
    , m_mainWin(mainWin)
    , m_isEditing(false)
    , m_originalMode(DisplayMode::NORMAL)
    , m_bookStart(0)
    , m_originalLen(0)
    , m_hFont(nullptr)
{
}

EditController::~EditController()
{
    if (m_hFont)
    {
        DeleteObject(m_hFont);
        m_hFont = nullptr;
    }
    close();
}

bool EditController::create()
{
    if (isValid()) return true;
    m_hWnd = CreateWindowExW(
        0, WC_EDIT, nullptr,
        WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_WANTRETURN,
        0, 0, 0, 0,
        m_hParentWnd, nullptr, m_hInstance, nullptr
    );
    SetWindowSubclass(m_hWnd, subclassProc, 0, (DWORD_PTR)this);
    return isValid();
}

void EditController::beginEdit()
{
    if (m_isEditing || !m_mainWin || !m_mainWin->m_session || !m_mainWin->m_session->isValid()) return;
    if (!isValid() && !create()) return;

    // exit borderless mode
    m_originalMode = Settings::instance().data().window.mode;
    if (m_originalMode != DisplayMode::NORMAL) m_mainWin->switchDisplayMode(DisplayMode::NORMAL);

    // disable menu
    m_mainWin->enableMenu(m_mainWin->m_hMenu, FALSE);

    BOOL readonly = !m_mainWin->m_session->book()->isEditSupport();
    if (m_hFont) DeleteObject(m_hFont);
    m_hFont = CreateFontIndirectW(&m_mainWin->m_data.styles.textFont);

    updateLayout();
    initOriginalText();
    if (m_hFont) SendMessage(m_hWnd, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    SendMessage(m_hWnd, WM_SETTEXT, 0, (LPARAM)m_originalEditText.c_str());
    SendMessage(m_hWnd, EM_SETREADONLY, (WPARAM)readonly, NULL);

    ShowWindow(m_hWnd, SW_SHOW);
    SetFocus(m_hWnd);
    m_isEditing = true;
    m_mainWin->updateStatusBar();
}

void EditController::endEdit()
{
    if (!isValid() || !m_isEditing || !m_mainWin || !m_mainWin->m_session || !m_mainWin->m_session->isValid()) return;

    bool needRedraw = false;
    wchar_t *buffer = nullptr;
    int length = (int)SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, NULL);
    if (length >= 0)
    {
        buffer = (wchar_t *)malloc(sizeof(wchar_t) * (length + 1));
        if (buffer)
        {
            buffer[length] = 0;
            if (length > 0) GetWindowText(m_hWnd, buffer, length + 1);

            if (0 != wcscmp(m_originalEditText.c_str(), buffer))
            {
                if (IDYES == messageBox(IDS_SAVE_TEXT_TIPS, IDS_SAVE_FILE, MB_ICONQUESTION | MB_YESNO))
                {
                    if (!m_mainWin->m_session->book()->editText(m_bookStart, m_originalLen, buffer, length))
                    {
                        messageBox(IDS_SAVE_FAIL, IDS_SAVE_FILE, MB_ICONERROR | MB_OK);
                    }
                    else
                    {
                        needRedraw = true;
                    }
                }
            }
            free(buffer);
        }
    }

    m_mainWin->enableMenu(m_mainWin->m_hMenu, TRUE);
    ShowWindow(m_hWnd, SW_HIDE);
    m_isEditing = false;
    if (m_originalMode != DisplayMode::NORMAL) m_mainWin->switchDisplayMode(m_originalMode);
    else if (needRedraw) m_mainWin->redrawWindow();
    else m_mainWin->updateStatusBar();
}

bool EditController::isEditing() const
{
    return m_isEditing;
}

void EditController::updateLayout()
{
    if (isValid() && m_mainWin)
    {
        RECT rc = m_mainWin->drawTextRect();
        SetWindowPos(m_hWnd, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOREDRAW);
    }
}

void EditController::initOriginalText()
{
    // '\n' -> '\r\n'
    m_bookStart = 0;
    m_originalLen = 0;
    m_originalEditText.clear();

    if (m_mainWin && m_mainWin->m_session && m_mainWin->m_session->isValid())
    {
        auto pageInfo = m_mainWin->m_session->pageInfo();
        m_bookStart = pageInfo.start;
        m_originalLen = pageInfo.length;
        if (m_bookStart >= 0 && m_originalLen > 0 && m_bookStart + m_originalLen <= m_mainWin->m_session->book()->length())
        {
            m_originalEditText.reserve(m_originalLen + pageInfo.lines.size());
            for (int i=m_bookStart; i<m_bookStart+m_originalLen; i++)
            {
                wchar_t ch = *(m_mainWin->m_session->book()->text() + i);
                if (ch == L'\n' && (m_originalEditText.empty() || m_originalEditText.back() != L'\r'))
                    m_originalEditText.push_back(L'\r');
                m_originalEditText.push_back(ch);
            }
        }
    }
}

LRESULT EditController::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    return -1;
}

LRESULT CALLBACK EditController::subclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    EditController* this_ = (EditController*)(dwRefData);
    switch (uMsg)
    {
    case WM_KEYDOWN:
        // ESC
        if (wParam == VK_ESCAPE)
        {
            this_->endEdit();
            return 0;
        }
        // Ctrl+S
        if ('S' == wParam && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
        {
            this_->endEdit();
            return 0;
        }
        // fast key
        for (auto &k : this_->m_mainWin->m_data.keyset.keys)
        {
            if (k.id == KeyId::EDIT_MODE)
            {
                if (k.enable && k.value == this_->m_mainWin->toModifiers(wParam))
                {
                    this_->endEdit();
                    return 0;
                }
                break;
            }
        }
        break;
    case WM_MOUSEWHEEL:
        SendMessage(hWnd, EM_LINESCROLL, 0, GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? -1 : 1);
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}