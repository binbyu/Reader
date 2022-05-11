#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.h"

// convert
wchar_t* ansi_to_utf16(const char* str, int size, int* len);
char* utf16_to_ansi(const wchar_t* str, int size, int* len);
wchar_t* utf8_to_utf16(const char* str, int size, int* len);
char* utf16_to_utf8(const wchar_t* str, int size, int* len);
char* utf16_to_utf8_bom(const wchar_t* str, int size, int* len);
void free_buffer(void* buffer);

char* Utf16ToUtf8(const wchar_t* str); // not free
char* Utf16ToAnsi(const wchar_t* str); // not free
wchar_t* Utf8ToUtf16(const char* str); // not free
void FreeConvertBuffer();

// encoding
type_t check_bom(const char *data, size_t size);
int is_ascii(const char *data, size_t size);    
int is_utf8(const char *data, size_t size);

// le be
char* le_to_be(char* data, int len);
char* be_to_le(char* data, int len);

// base64
void b64_encode(const char *src, int slen, char *dst, int *dlen);
void b64_decode(const char *src, int slen, char *dst, int *dlen);

// url
int url_encode(const char *src, char *dest);
int url_decode(const char *src, char *dest); 

// string
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, int n);
char *strcasestr(const char *s, const char *find);

// check system version
BOOL Is_WinXP_SP2_or_Later(void);

void GetApplicationVersion(TCHAR *version);

int memvcmp(void *memory, unsigned char val, unsigned int size);

#endif