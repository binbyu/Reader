// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO: reference additional headers your program requires here
#ifdef  FD_SETSIZE
#undef  FD_SETSIZE
#define FD_SETSIZE  1024
#else 
#define FD_SETSIZE  1024
#endif

#define ZLIB_ENABLE

typedef struct IUnknown IUnknown;
#include <objidl.h>
#include <GdiPlus.h>
#pragma comment(lib, "Gdiplus.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Shobjidl.h>

#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "Version.lib")

#ifdef _DEBUG
#ifdef ZLIB_ENABLE
#pragma comment(lib, "zlibstatd.lib")
#endif
#pragma comment(lib, "libxml2_ad.lib")
#pragma comment(lib, "libmobid.lib")
#ifdef ENABLE_NETWORK
#pragma comment(lib, "libhttps_ad.lib")
#pragma comment(lib, "wolfssld.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Ws2_32.lib")
#endif
#else
#ifdef ZLIB_ENABLE
#pragma comment(lib, "zlibstat.lib")
#endif
#pragma comment(lib, "libxml2_a.lib")
#pragma comment(lib, "libmobi.lib")
#ifdef ENABLE_NETWORK
#pragma comment(lib, "libhttps_a.lib")
#pragma comment(lib, "wolfssl.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Ws2_32.lib")
#endif
#endif


#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
