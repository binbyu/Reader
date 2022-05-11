#ifndef __DUMP_H__
#define __DUMP_H__
#ifdef _DEBUG

void CreateDumpFile(LPCWSTR lpstrDumpFilePathName, EXCEPTION_POINTERS *pException);
LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException);

void logger_create(void);
void logger_destroy(void);
void __stdcall logger_printf(char const* const format, ...);

#endif
#endif
