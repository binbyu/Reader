#pragma once

#include "framework.h"
#include "common.h"
#include "Utils.h"
#include <string>

class SingleApp
{
private:
    struct EnumParam
    {
        const wchar_t* targetClassName;
        HWND* phWnd;
    };

public:
    SingleApp(const wchar_t* className);
    ~SingleApp();

    bool isRunning() const;
    void activate();

private:
    std::wstring mutexName();

private:
    static BOOL CALLBACK enumWindowsProc(HWND hWnd, LPARAM lParam);

private:
    std::wstring m_className;
    HANDLE       m_hMutex;
    bool         m_isRunning;
};
