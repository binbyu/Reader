#pragma once
#include "IWnd.h"

struct WinConfig
{
    std::wstring className;
    std::wstring title;
    LPCWSTR menuName;
    LPCWSTR iconName;
    LPCWSTR iconSmName;
    DWORD dwStyle;
    DWORD dwExStyle;
    RECT rect;
};

class Window : public IWnd
{
public:
    Window(HINSTANCE hInstance, const WinConfig& config, HWND hParent = nullptr)
        : IWnd(hInstance, hParent)
        , m_config(config)
    {
    }

    virtual ~Window()
    {
        close();
        UnregisterClass(m_config.className.c_str(), m_hInstance);
    }

protected:
    BOOL create()
    {
        registerWindow();
        return createWindow();
    }

private:
    ATOM registerWindow()
    {
        WNDCLASSEXW wcex;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = wndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = m_hInstance;
        wcex.hIcon = LoadIcon(m_hInstance, m_config.iconName);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = m_config.menuName;
        wcex.lpszClassName = m_config.className.c_str();
        wcex.hIconSm = LoadIcon(wcex.hInstance, m_config.iconSmName);
        return RegisterClassExW(&wcex);
    }

    BOOL createWindow()
    {
        prevCreate();
        m_hWnd = CreateWindowEx(
            m_config.dwExStyle,
            m_config.className.c_str(),
            m_config.title.c_str(),
            m_config.dwStyle,
            m_config.rect.left,
            m_config.rect.top,
            m_config.rect.right - m_config.rect.left,
            m_config.rect.bottom - m_config.rect.top,
            nullptr,
            nullptr,
            m_hInstance,
            this);
        postCreate();
        return isValid();
    }

    static LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Window* pThis = nullptr;
        if (WM_NCCREATE == message)
        {
            CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
            pThis = reinterpret_cast<Window*>(pcs->lpCreateParams);
            setThis(hWnd, pThis);
            pThis->m_hWnd = hWnd;
        }
        else
        {
            pThis = getThis<Window>(hWnd);
        }

        LRESULT result = -1;
        if (pThis)
        {
            result = pThis->handleMessage(message, wParam, lParam);
        }
        if (-1 == result)
        {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return result;
    }

protected:
    virtual void prevCreate() {}
    virtual void postCreate() {}

protected:
    const WinConfig& m_config;
};