#include "stdafx.h"
#include "dump.h"

#ifdef _DEBUG
#include <DbgHelp.h>

void CreateDumpFile(LPCWSTR lpstrDumpFilePathName, EXCEPTION_POINTERS *pException)
{
    typedef BOOL (WINAPI * MiniDumpWriteDump_t)(
        __in HANDLE,
        __in DWORD,
        __in HANDLE,
        __in MINIDUMP_TYPE,
        __in_opt PMINIDUMP_EXCEPTION_INFORMATION,
        __in_opt PMINIDUMP_USER_STREAM_INFORMATION,
        __in_opt PMINIDUMP_CALLBACK_INFORMATION
        );

    HANDLE hDumpFile = NULL;
    HMODULE hDbgHelp = NULL;
    MiniDumpWriteDump_t fpMiniDumpWriteDump = NULL;
    MINIDUMP_EXCEPTION_INFORMATION dumpInfo = {0};

    dumpInfo.ExceptionPointers = pException;  
    dumpInfo.ThreadId = GetCurrentThreadId();  
    dumpInfo.ClientPointers = TRUE;

    hDbgHelp = LoadLibrary(L"DbgHelp.dll");
    if (hDbgHelp)
        fpMiniDumpWriteDump = (MiniDumpWriteDump_t)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");

    if (fpMiniDumpWriteDump)
    {
        hDumpFile = CreateFile(lpstrDumpFilePathName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        fpMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
    }

    if (hDbgHelp != NULL)
        FreeLibrary(hDbgHelp);
}

LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException)
{
    SYSTEMTIME tm;
    GetLocalTime(&tm);
    TCHAR fileName[255] = {0};
    _stprintf(fileName, _T("Reader_Crash_%04d_%02d_%02d_%02d_%02d_%02d_%03d.dmp"),
        tm.wYear,tm.wMonth,tm.wDay, tm.wHour,tm.wMinute,tm.wSecond,tm.wMilliseconds);

    CreateDumpFile(fileName, pException);  
    FatalAppExit(-1,  L"*** Unhandled Exception! ***");  

    return EXCEPTION_EXECUTE_HANDLER;  
}
#endif
