#include "SingleApp.h"
#include "Utils.h"
#include <algorithm>

SingleApp::SingleApp(const wchar_t* className)
    : m_className(className)
    , m_isRunning(true)
{
    std::wstring name = mutexName();
    m_hMutex = OpenMutexW(MUTEX_ALL_ACCESS, TRUE, name.c_str());
    auto err = GetLastError();
    if (!m_hMutex && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        m_hMutex = CreateMutexW(NULL, TRUE, name.c_str());
        m_isRunning = false;
    }
}

SingleApp::~SingleApp()
{
    if (m_hMutex)
    {
        if (!m_isRunning)
        {
            ReleaseMutex(m_hMutex);
        }
        CloseHandle(m_hMutex);
        m_hMutex = nullptr;
    }
}

bool SingleApp::isRunning() const
{
    return m_isRunning;
}

std::wstring SingleApp::cmdFile()
{
    std::wstring result;
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv)
    {
        if (argc > 1)
        {
            wchar_t fullPath[MAX_PATH] = { 0 };
            GetFullPathNameW(argv[1], MAX_PATH, fullPath, NULL);
            result = fullPath;
        }
        LocalFree(argv);
    }
    return result;
}

void SingleApp::activate()
{
    HWND hWnd = NULL;
    EnumParam param = {m_className.c_str(), &hWnd};
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&param));

    if (hWnd)
    {
        std::wstring fileName = cmdFile();
        if (!fileName.empty())
        {
            COPYDATASTRUCT copyData = {0};
            copyData.dwData = 0;
            copyData.cbData = (DWORD)((fileName.size() + 1) * sizeof(wchar_t));
            copyData.lpData = (PVOID)fileName.c_str();
            SendMessageW(hWnd, WM_COPYDATA, 0, (LPARAM)&copyData);
        }
    }
}

std::wstring SingleApp::mutexName()
{
    std::wstring path = Utils::appPath();
    std::wstring hash = Utils::fnv1a32(path.data(), path.size() * sizeof(wchar_t));
    return L"Reader_" + hash;
}

BOOL CALLBACK SingleApp::enumWindowsProc(HWND hWnd, LPARAM lParam)
{
    EnumParam* param = reinterpret_cast<EnumParam*>(lParam);
    TCHAR szClassName[MAX_LOADSTRING] = {0};

    GetClassName(hWnd, szClassName, MAX_LOADSTRING);
    if (wcscmp(szClassName, param->targetClassName) == 0)
    {
        HWND hForeWnd = GetForegroundWindow();
        DWORD dwCurID = GetCurrentThreadId();
        DWORD dwForeID = GetWindowThreadProcessId(hForeWnd, NULL);
        AttachThreadInput(dwCurID, dwForeID, TRUE);
        ShowWindow(hWnd, SW_SHOWNORMAL);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        SetForegroundWindow(hWnd);
        AttachThreadInput(dwCurID, dwForeID, FALSE);
        *param->phWnd = hWnd;
        return FALSE;
    }
    return TRUE;
}