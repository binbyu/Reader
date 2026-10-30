#pragma once
#include "IWnd.h"
#include "conv.h"

class Dialog : public IWnd
{
public:
    Dialog(HINSTANCE hInstance, HWND hParent, UINT resourceId)
        : IWnd(hInstance, hParent)
        , m_resourceId(resourceId)
    {
    }

    virtual ~Dialog()
    {
        close();
    }

    virtual void create()
    {
        if (!m_hWnd)
        {
            m_hWnd = CreateDialogParamW(m_hInstance, MAKEINTRESOURCE(m_resourceId), m_hParentWnd, dlgProc, (LPARAM)this);
        }
    }

    virtual void show(int cmd = SW_SHOW)
    {
        create();
        ShowWindow(m_hWnd, cmd);
    }

    INT_PTR exec()
    {
        INT_PTR result = DialogBoxParamW(m_hInstance, MAKEINTRESOURCE(m_resourceId), m_hParentWnd, dlgProc, (LPARAM)this);
        m_hWnd = nullptr;
        return result;
    }

    std::wstring getDlgItemTextW(int itemId)
    {
        wchar_t buffer[MAX_URL_LENGTH] = {0};
        GetDlgItemTextW(m_hWnd, itemId, buffer, sizeof(buffer));
        return std::wstring(buffer);
    }

    std::string getDlgItemTextA(int itemId)
    {
        wchar_t buffer[MAX_URL_LENGTH] = {0};
        GetDlgItemTextW(m_hWnd, itemId, buffer, sizeof(buffer));
        return toUtf8(buffer);
    }

protected:
    virtual void centerParent(const RECT& rect)
    {
        RECT rcChild = rect;
        RECT rcParent;
        if (m_hParentWnd)
        {
            GetWindowRect(m_hParentWnd, &rcParent);
        }
        else
        {
            SystemParametersInfo(SPI_GETWORKAREA, 0, &rcParent, 0);
        }
        int pw = rcParent.right - rcParent.left;
        int ph = rcParent.bottom - rcParent.top;
        int cw = rcChild.right - rcChild.left;
        int ch = rcChild.bottom - rcChild.top;

        rcChild.left = rcParent.left + (pw - cw) / 2;
        rcChild.top = rcParent.top + (ph - ch) / 2;
        rcChild.right = rcChild.left + cw;
        rcChild.bottom = rcChild.top + ch;
        SetWindowPos(m_hWnd, NULL, rcChild.left, rcChild.top, rcChild.right - rcChild.left, rcChild.bottom - rcChild.top, SWP_NOREDRAW);
    }

    virtual void centerParent()
    {
        RECT rect;
        GetWindowRect(m_hWnd, &rect);
        centerParent(rect);
    }

private:
    static INT_PTR CALLBACK dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Dialog* pThis = nullptr;
        if (WM_INITDIALOG == message)
        {
            pThis = reinterpret_cast<Dialog*>(lParam);
            setThis(hDlg, pThis);
            pThis->m_hWnd = hDlg;
        }
        else
        {
            pThis = getThis<Dialog>(hDlg);
        }
        if (!pThis)
        {
            return FALSE;
        }
        return pThis->handleMessage(message, wParam, lParam);
    }

protected:
    UINT m_resourceId = 0;
};