#include "framework.h"
#include "dump.h"
#ifdef _DEBUG
#include <DbgHelp.h>
#include <stdio.h>
#include "Utils.h"

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
    TCHAR fileName[256] = {0};
    _stprintf(fileName, _T("Reader_Crash_%04d_%02d_%02d_%02d_%02d_%02d_%03d.dmp"),
        tm.wYear,tm.wMonth,tm.wDay, tm.wHour,tm.wMinute,tm.wSecond,tm.wMilliseconds);

    CreateDumpFile(fileName, pException);  
    FatalAppExit((UINT)-1,  L"*** Unhandled Exception! ***");  

    return EXCEPTION_EXECUTE_HANDLER;  
}

static FILE* _flogger = NULL;
static CRITICAL_SECTION _cs = {0};
static const int _LOG_BUFFER_SIZE = 1024;
static char *_log_buffer = NULL;
void logger_create(void)
{
    char filename[256] = { 0 };
    SYSTEMTIME tm;

    if (!_cs.DebugInfo)
    {
        InitializeCriticalSection(&_cs);
    }

    if (IsDebuggerPresent())
    {
        if (!_log_buffer)
        {
            _log_buffer = (char*)malloc(_LOG_BUFFER_SIZE);
        }
    }
    else
    {
        if (!_flogger)
        {
            GetLocalTime(&tm);
            sprintf(filename, "Reader_%04d_%02d_%02d.log", tm.wYear, tm.wMonth, tm.wDay);
            _flogger = fopen(filename, "wb+");
        }
    }
}

void logger_destroy(void)
{
    if (_flogger)
    {
        fclose(_flogger);
        _flogger = NULL;
    }
    if (_log_buffer)
    {
        free(_log_buffer);
        _log_buffer = NULL;
    }
    if (_cs.DebugInfo)
    {
        DeleteCriticalSection(&_cs);
        memset(&_cs, 0, sizeof(_cs));
    }
}

void __stdcall logger_printf(char const* const format, ...)
{
    va_list args;

    EnterCriticalSection(&_cs);
    va_start(args, format);
    if (_flogger)
    {
        vfprintf(_flogger, format, args);
        fflush(_flogger);
    }
    else if (_log_buffer)
    {
        vsnprintf(_log_buffer, _LOG_BUFFER_SIZE-1, format, args);
        OutputDebugStringA(_log_buffer);
    }
    va_end(args);
    LeaveCriticalSection(&_cs);
}

#endif
