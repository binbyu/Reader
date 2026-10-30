#include "framework.h"
#include "resource.h"
#include "MainWin.h"
#include "SingleApp.h"

BOOL initApp()
{
    CoInitialize(NULL);
    InitCommonControls();
    return TRUE;
}

void cleanup()
{
    CoUninitialize();
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    SingleApp app(MainWin::className(hInstance));
    if (app.isRunning())
    {
        app.activate();
        return 0;
    }

    if (!initApp())
    {
        cleanup();
        return -1;
    }

    MainWin win(hInstance);

    MSG msg;
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_READER));
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        HWND hFindDlg = win.hFindDlg();
        if (!hFindDlg || !IsDialogMessage(hFindDlg, &msg))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    cleanup();
    return static_cast<int>(msg.wParam);
}