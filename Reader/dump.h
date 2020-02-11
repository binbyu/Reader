#ifndef __DUMP_H__
#define __DUMP_H__

#ifdef _DEBUG

void CreateDumpFile(LPCWSTR lpstrDumpFilePathName, EXCEPTION_POINTERS *pException);
LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException);

#endif

#endif
