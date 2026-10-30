#pragma once
#include "framework.h"
#include "common.h"
#include "conv.h"
#include <string>

class IWnd
{
public:
    IWnd(HINSTANCE hInstance, HWND hParent = nullptr) : m_hInstance(hInstance), m_hParentWnd(hParent) {}
    virtual ~IWnd() = default;
    HINSTANCE hInstance() const { return m_hInstance; }
    HWND parentHWND() const { return m_hParentWnd; }
    HWND getHWND() const { return m_hWnd; }
    bool isValid() const { return m_hWnd != nullptr && IsWindow(m_hWnd); }
    virtual void show(int cmd = SW_SHOW) { ShowWindow(m_hWnd, cmd); }
    void hide() { ShowWindow(m_hWnd, SW_HIDE); }
    void enable(bool enable = true) { EnableWindow(m_hWnd, enable); }
    void close() { if (isValid()) { DestroyWindow(m_hWnd); m_hWnd = nullptr; } }

public://utils
    std::wstring toUtf16(const std::string& v)
    {
        std::wstring s;
        wchar_t* t = nullptr;
        conv_utf8_to_utf16(v.c_str(), (int)v.size(), &t);
        if (t)
        {
            s = t;
            conv_free(t);
        }
        return s;
    }

    std::string toUtf8(const std::wstring& v)
    {
        std::string s;
        char* t = nullptr;
        conv_utf16_to_utf8(v.c_str(), (int)v.size(), &t);
        if (t)
        {
            s = t;
            conv_free(t);
        }
        return s;
    }

    std::string toAnsi(const std::wstring& v)
    {
        std::string s;
        char* t = nullptr;
        conv_utf16_to_ansi(v.c_str(), (int)v.size(), &t);
        if (t)
        {
            s = t;
            conv_free(t);
        }
        return s;
    }

    static std::wstring loadString(HINSTANCE hInstance, UINT uID)
    {
        WCHAR szBuffer[MAX_LOADSTRING] = { 0 };
        LoadStringW(hInstance, uID, szBuffer, MAX_LOADSTRING);
        return std::wstring(szBuffer);
    }

    std::wstring loadString(UINT uID)
    {
        return loadString(m_hInstance, uID);
    }

    int messageBox(const wchar_t* text, const wchar_t* caption, UINT uType)
    {
        static HHOOK hMsgBoxHook = nullptr;
        auto MsgBoxHookProc = [](int code, WPARAM wParam, LPARAM lParam)
            {
                HWND hParent;
                RECT rc;
                int w, h, x, y;
                LPCBT_CREATEWND cbt;
                if (code == HCBT_CREATEWND)
                {
                    cbt = ((LPCBT_CREATEWND)lParam);

                    if (cbt->lpcs->lpszClass == (LPWSTR)(ATOM)32770) // #32770 = dialog box class
                    {
                        hParent = cbt->lpcs->hwndParent;
                        if (hParent)
                        {
                            GetWindowRect(hParent, &rc);
                            w = rc.right - rc.left;
                            h = rc.bottom - rc.top;
                            x = rc.left + (w - cbt->lpcs->cx) / 2;
                            y = rc.top + (h - cbt->lpcs->cy) / 2;
                            cbt->lpcs->x = x;
                            cbt->lpcs->y = y;
                        }
                    }
                }
                return CallNextHookEx(hMsgBoxHook, code, wParam, lParam);
            };

        hMsgBoxHook = SetWindowsHookEx(WH_CBT, MsgBoxHookProc, m_hInstance, GetCurrentThreadId());
        int res = MessageBoxEx(m_hWnd, text, caption, uType, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

        if (hMsgBoxHook)
        {
            UnhookWindowsHookEx(hMsgBoxHook);
            hMsgBoxHook = nullptr;
        }
        return res;
    }

    int messageBox(UINT textId, UINT captionId, UINT uType)
    {
        return messageBox(loadString(textId).c_str(), loadString(captionId).c_str(), uType);
    }

    int messageBoxFmt(UINT textFormatId, UINT captionId, UINT uType, ...)
    {
        WCHAR szText[MAX_LOADSTRING] = { 0 };
        std::wstring format = loadString(textFormatId);
        va_list args;
        va_start(args, uType);
        _vsnwprintf(szText, MAX_LOADSTRING, format.c_str(), args);
        va_end(args);
        return messageBox(szText, loadString(captionId).c_str(), uType);
    }

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) = 0;
    template<typename T> static T* getThis(HWND hWnd) { return reinterpret_cast<T*>(GetWindowLongPtr(hWnd, GWLP_USERDATA)); }
    template<typename T> static void setThis(HWND hWnd, T* pThis) { SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis)); }

protected:
    HINSTANCE m_hInstance = nullptr;
    HWND m_hParentWnd = nullptr;
    HWND m_hWnd = nullptr;
};