#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.h"

class Utils
{
public:
    Utils(void);
    ~Utils(void);

public:
#if ENABLE_MD5
    // md5
    static bool get_md5(void* data, size_t size, u128_t* result);
#endif

    // convert
    static wchar_t* ansi_to_utf16(const char* str, int* len);
    static wchar_t* ansi_to_utf16_ex(const char* str, int size, int* len);
    static char* utf16_to_ansi(const wchar_t* str, int* len);
    static wchar_t* utf8_to_utf16(const char* str, int* len);
    static wchar_t* utf8_to_utf16_ex(const char* str, int size, int* len);
    static char* utf16_to_utf8(const wchar_t* str, int* len);
    static void free_buffer(void* buffer);

    // encoding
    static type_t check_bom(const char *data, size_t size);
    static int is_ascii(const char *data, size_t size);    
    static int is_utf8(const char *data, size_t size);

    // le be
    static char* le_to_be(char* data, int len);
    static char* be_to_le(char* data, int len);

    // base64
    static void b64_encode(const char *src, int slen, char *dst, int *dlen);
    static void b64_decode(const char *src, int slen, char *dst, int *dlen);

    // check system version
    static BOOL isWindowsXP(void);
};

#endif