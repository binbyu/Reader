#include "StdAfx.h"
#include "Utils.h"
#include <stdint.h>
#include <Wincrypt.h>
#include <string.h>
#include <stdio.h>
#include "zlib.h"


Utils::Utils(void)
{
}


Utils::~Utils(void)
{
}

#if ENABLE_MD5
bool Utils::get_md5(void* data, size_t size, u128_t* result)
{
    HCRYPTPROV hProv = NULL;
    HCRYPTPROV hHash = NULL;
    DWORD cbHashSize = 0;
    DWORD dwCount = sizeof(DWORD);
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        return false;
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        CryptReleaseContext(hProv, 0);
        return false;
    }

    if (!CryptHashData(hHash, (BYTE*)data, size, 0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return false;
    }

    if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&cbHashSize, &dwCount, 0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return false;
    }

    if (!CryptGetHashParam(hHash, HP_HASHVAL, (BYTE*)result, &cbHashSize, 0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return false;
    }
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return true;
}
#endif

wchar_t* Utils::ansi_to_utf16(const char* str, int* len)
{
    wchar_t* result;
    *len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    result = (wchar_t*)malloc((*len)*sizeof(wchar_t));
    memset(result, 0, (*len)*sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, str, -1, (LPWSTR)result, *len);
    return result;
}

wchar_t* Utils::ansi_to_utf16_ex(const char* str, int size, int* len)
{
    wchar_t* result;
    *len = MultiByteToWideChar(CP_ACP, 0, str, size, NULL, 0);
    result = (wchar_t*)malloc(((*len)+1)*sizeof(wchar_t));
    memset(result, 0, ((*len)+1)*sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, str, size, (LPWSTR)result, *len);
    return result;
}

char* Utils::utf16_to_ansi(const wchar_t* str, int* len)
{
    char* result;
    *len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    result = (char*)malloc((*len)*sizeof(char));
    memset(result, 0, (*len)*sizeof(char));
    WideCharToMultiByte(CP_ACP, 0, str, -1, result, *len, NULL, NULL);
    return result;
}

wchar_t* Utils::utf8_to_utf16(const char* str, int* len)
{
    wchar_t* result;
    *len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    result = (wchar_t*)malloc((*len)*sizeof(wchar_t));
    memset(result, 0, (*len)*sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, str, -1, (LPWSTR)result, *len);
    return result;
}

wchar_t* Utils::utf8_to_utf16_ex(const char* str, int size, int* len)
{
    wchar_t* result;
    *len = MultiByteToWideChar(CP_UTF8, 0, str, size, NULL, 0);
    result = (wchar_t*)malloc(((*len)+1) * sizeof(wchar_t));
    memset(result, 0, ((*len)+1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, str, size, (LPWSTR)result, *len);
    return result;
}

char* Utils::utf16_to_utf8(const wchar_t* str, int* len)
{
    char* result;
    *len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    result =(char*)malloc((*len)*sizeof(char));
    memset(result, 0, (*len)*sizeof(char));
    WideCharToMultiByte(CP_UTF8, 0, str, -1, result, *len, NULL, NULL);
    return result;
}

void Utils::free_buffer(void* buffer)
{
    if (buffer)
    {
        free(buffer);
    }
}


char* Utils::g_result = NULL;
int Utils::g_len = 0;
wchar_t* Utils::g_wresult = NULL;
int Utils::g_wlen = 0;

char* Utils::Utf16ToUtf8(const wchar_t* str)
{
    int len;

    len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    if (!g_result)
    {
        g_len = len + 1;
        g_result = (char*)malloc(g_len * sizeof(char));
    }
    else if (g_len < len + 1)
    {
        g_len = len + 1;
        g_result = (char*)realloc(g_result, g_len * sizeof(char));
    }

    WideCharToMultiByte(CP_UTF8, 0, str, -1, g_result, len, NULL, NULL);
    g_result[len] = 0;
    return g_result;
}

wchar_t* Utils::Utf8ToUtf16(const char* str)
{
    int len;

    len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (!g_wresult)
    {
        g_wlen = len + 1;
        g_wresult = (wchar_t*)malloc(g_wlen * sizeof(wchar_t));
    }
    else if (g_wlen < len + 1)
    {
        g_wlen = len + 1;
        g_wresult = (wchar_t*)realloc(g_wresult, g_wlen * sizeof(wchar_t));
    }

    MultiByteToWideChar(CP_UTF8, 0, str, -1, (LPWSTR)g_wresult, len);
    g_wresult[len] = 0;
    return g_wresult;
}

void Utils::FreeConvertBuffer()
{
    if (g_result)
    {
        free(g_result);
        g_result = NULL;
    }
    if (g_wresult)
    {
        free(g_wresult);
        g_wresult = NULL;
    }
    g_len = 0;
    g_wlen = 0;
}

const char *UTF_16_BE_BOM = "\xFE\xFF";
const char *UTF_16_LE_BOM = "\xFF\xFE";
const char *UTF_8_BOM = "\xEF\xBB\xBF";
const char *UTF_32_BE_BOM = "\x00\x00\xFE\xFF";
const char *UTF_32_LE_BOM = "\xFF\xFE\x00\x00";

type_t Utils::check_bom(const char *data, size_t size)
{
    if (size >= 3) {
        if (memcmp(data, UTF_8_BOM, 3) == 0)
            return utf8;
    }
    if (size >= 4) {
        if (memcmp(data, UTF_32_LE_BOM, 4) == 0)
            return utf32_le;
        if (memcmp(data, UTF_32_BE_BOM, 4) == 0)
            return utf32_be;
    }
    if (size >= 2) {
        if (memcmp(data, UTF_16_LE_BOM, 2) == 0)
            return utf16_le;
        if (memcmp(data, UTF_16_BE_BOM, 2) == 0)
            return utf16_be;
    }
    return Unknown;
}

int Utils::is_ascii(const char *data, size_t size)
{
    const unsigned char *str = (const unsigned char*)data;
    const unsigned char *end = str + size;
    for (; str != end; str++) {
        if (*str & 0x80)
            return 0;
    }
    return 1;
}

int Utils::is_utf8(const char *data, size_t size)
{
    const unsigned char *str = (unsigned char*)data;
    const unsigned char *end = str + size;
    unsigned char byte;
    unsigned int code_length, i;
    uint32_t ch;
    while (str != end) {
        byte = *str;
        if (byte <= 0x7F) {
            /* 1 byte sequence: U+0000..U+007F */
            str += 1;
            continue;
        }

        if (0xC2 <= byte && byte <= 0xDF)
            /* 0b110xxxxx: 2 bytes sequence */
            code_length = 2;
        else if (0xE0 <= byte && byte <= 0xEF)
            /* 0b1110xxxx: 3 bytes sequence */
            code_length = 3;
        else if (0xF0 <= byte && byte <= 0xF4)
            /* 0b11110xxx: 4 bytes sequence */
            code_length = 4;
        else {
            /* invalid first byte of a multibyte character */
            return 0;
        }

        if (str + (code_length - 1) >= end) {
            /* truncated string or invalid byte sequence */
            break;//return 0; fixed bug.... substr
        }

        /* Check continuation bytes: bit 7 should be set, bit 6 should be
         * unset (b10xxxxxx). */
        for (i=1; i < code_length; i++) {
            if ((str[i] & 0xC0) != 0x80)
                return 0;
        }

        if (code_length == 2) {
            /* 2 bytes sequence: U+0080..U+07FF */
            ch = ((str[0] & 0x1f) << 6) + (str[1] & 0x3f);
            /* str[0] >= 0xC2, so ch >= 0x0080.
               str[0] <= 0xDF, (str[1] & 0x3f) <= 0x3f, so ch <= 0x07ff */
        } else if (code_length == 3) {
            /* 3 bytes sequence: U+0800..U+FFFF */
            ch = ((str[0] & 0x0f) << 12) + ((str[1] & 0x3f) << 6) +
                  (str[2] & 0x3f);
            /* (0xff & 0x0f) << 12 | (0xff & 0x3f) << 6 | (0xff & 0x3f) = 0xffff,
               so ch <= 0xffff */
            if (ch < 0x0800)
                return 0;

            /* surrogates (U+D800-U+DFFF) are invalid in UTF-8:
               test if (0xD800 <= ch && ch <= 0xDFFF) */
            if ((ch >> 11) == 0x1b)
                return 0;
        } else if (code_length == 4) {
            /* 4 bytes sequence: U+10000..U+10FFFF */
            ch = ((str[0] & 0x07) << 18) + ((str[1] & 0x3f) << 12) +
                 ((str[2] & 0x3f) << 6) + (str[3] & 0x3f);
            if ((ch < 0x10000) || (0x10FFFF < ch))
                return 0;
        }
        str += code_length;
    }
    return 1;
}

char* Utils::le_to_be(char* data, int len)
{
    char tmp;
    for (int i=0; i<len-1; i+=2)
    {
        tmp = data[i];
        data[i] = data[i+1];
        data[i+1] = tmp;
    }
    return data;
}

char* Utils::be_to_le(char* data, int len)
{
    return le_to_be(data, len);
}

void Utils::b64_encode(const char *src, int slen, char *dst, int *dlen)
{
    static const char b64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    int i;
    unsigned i_bits = 0;
    int i_shift = 0;
    int out_len = 0;

    for (i = 0; i < slen; i++)
    {
        i_bits = (i_bits << 8) | src[i];
        i_shift += 8;
        while (i_shift >= 6)
        {
            dst[out_len++] = b64[(i_bits << 6 >> i_shift) & 0x3f];
            i_shift -= 6;
        }
    }
    while (i_shift > 0)
    {
        dst[out_len++] = b64[(i_bits << 6 >> i_shift) & 0x3f];
        i_shift -= 6;
    }
    while (out_len & 3)
    {
        dst[out_len++] = '=';
    }
    *dlen = out_len;
}

void Utils::b64_decode(const char *src, int slen, char *dst, int *dlen)
{
    static const unsigned char ascii[256] =
    {
        /* ASCII table */
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };

    int i;
    unsigned i_bits = 0;
    int i_shift = 0;
    int out_len = 0;

    for (i = 0; i < slen; i++)
    {
        if (ascii[src[i]] == 64)
            break;
        i_bits = (i_bits << 6) | ascii[src[i]];
        i_shift += 6;
        while (i_shift >= 8)
        {
            dst[out_len++] = (i_bits << 8 >> i_shift) & 0xff;
            i_shift -= 8;
        }
    }
    *dlen = out_len;
}

BOOL Utils::Is_WinXP_SP2_or_Later(void)
{
    OSVERSIONINFOEX osvi;
    DWORDLONG dwlConditionMask = 0;
    int op = VER_GREATER_EQUAL;

    // Initialize the OSVERSIONINFOEX structure.

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = 5;
    osvi.dwMinorVersion = 1;
    osvi.wServicePackMajor = 2;
    osvi.wServicePackMinor = 0;

    // Initialize the condition mask.

    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);
    VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, op);
    VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMINOR, op);

    // Perform the test.

    return VerifyVersionInfo(
        &osvi,
        VER_MAJORVERSION | VER_MINORVERSION |
        VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        dwlConditionMask);
}

void Utils::UrlEncode(const char* src, char** dst)
{
    static char rfc3986[256] = { 0 };
    static char html5[256] = { 0 };
    static int i = 0;

    char* enc = NULL;
    unsigned char* s = (unsigned char*)src;
    int len = strlen(src);
    *dst = (char*)malloc((len * 3) + 1);
    memset(*dst, 0, (len * 3) + 1);
    enc = *dst;

    // init
    if (i == 0)
    {
        for (i = 0; i < 256; i++) {
            rfc3986[i] = isalnum(i) || i == '~' || i == '-' || i == '.' || i == '_'
                ? i : 0;
            html5[i] = isalnum(i) || i == '*' || i == '-' || i == '.' || i == '_'
                ? i : (i == ' ') ? '+' : 0;
        }
    }

    // encode
    for (; *s; s++) {
        if (html5[*s])
            sprintf(enc, "%c", html5[*s]);
        else
            sprintf(enc, "%%%02X", *s);
        while (*++enc);
    }

    //free(*dst);
}

#define ishex(x) ((x >= '0' && x <= '9') || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F'))
void Utils::UrlDecode(const char* src, char** dst)
{
    int len = strlen(src);
    char* out = NULL;
    char* o = out;
    const char* s = src;
    const char* end = s + (len - 1);
    int c;

    *dst = NULL;
    out = (char*)malloc(len + 1);
    if (!out)
    {
        return;
    }

    for (o = out; s <= end; o++)
    {
        c = *s++;
        if (c == '+')
        {
            c = ' ';
        }
        else if (c == '%'
            && (!ishex(*s++) || !ishex(*s++) || !sscanf(s - 2, "%2x", &c)))
        {
            // bad uri
            free(out);
            return;
        }

        if (out)
        {
            *o = c;
        }
    }

    *dst = out;
    //free(out);
}

void Utils::UrlFree(char* url)
{
    if (url)
        free(url);
}

BOOL Utils::gzipInflate(const unsigned char* src, int srclen, unsigned char** dst, int* dstlen)
{
    z_stream stream = { 0 };
    bool done = false;
    unsigned char* buf = NULL;
    int buflen = 0;
    int err;

    *dst = NULL;
    *dstlen = 0;
    if (srclen <= 0)
        return FALSE;

    stream.next_in = (Bytef*)src;
    stream.avail_in = srclen;
    stream.total_out = 0;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;

    if (inflateInit2(&stream, (16 + MAX_WBITS)) != Z_OK)
        return FALSE;

    buflen = srclen + srclen / 2;
    buf = (unsigned char*)malloc(buflen);

    while (1)
    {
        if ((int)stream.total_out >= buflen)
        {
            // increase output buffer  
            buflen += srclen / 2;
            buf = (unsigned char*)realloc(buf, buflen);
        }

        stream.next_out = buf + stream.total_out;
        stream.avail_out = buflen - stream.total_out;

        // Inflate next chunk
        err = inflate(&stream, Z_SYNC_FLUSH);
        if (err == Z_STREAM_END)
        {
            err = Z_OK;
            break;
        }
        else if (err != Z_OK)
        {
            break;
        }
    }

    if (inflateEnd(&stream) != Z_OK)
    {
        free(buf);
        return FALSE;
    }

    if (err != Z_OK)
    {
        free(buf);
        return FALSE;
    }

    *dst = buf;
    *dstlen = stream.total_out;
    return TRUE;
}

void Utils::GetApplicationVersion(TCHAR* version)
{
    TCHAR szModPath[MAX_PATH] = { 0 };
    DWORD dwHandle;
    DWORD dwSize;
    BYTE* pBlock;
    TCHAR SubBlock[MAX_PATH] = { 0 };
    TCHAR* buffer;
    UINT cbTranslate;

    if (!version)
        return;

    *version = 0;
    GetModuleFileName(NULL, szModPath, sizeof(TCHAR) * (MAX_PATH - 1));
    dwSize = GetFileVersionInfoSize(szModPath, &dwHandle);
    if (dwSize > 0)
    {
        pBlock = (BYTE*)malloc(dwSize);
        memset(pBlock, 0, dwSize);
        if (GetFileVersionInfo(szModPath, dwHandle, dwSize, pBlock))
        {
            struct LANGANDCODEPAGE {
                WORD wLanguage;
                WORD wCodePage;
            } *lpTranslate;

            // Read the list of languages and code pages.
            if (VerQueryValue(pBlock, _T("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate))
            {
                // Read the file description for each language and code page.
                _stprintf(SubBlock, _T("\\StringFileInfo\\%04x%04x\\FileVersion"), lpTranslate->wLanguage, lpTranslate->wCodePage);
                if (VerQueryValue(pBlock, SubBlock, (LPVOID*)&buffer, &cbTranslate))
                {
                    _tcscpy(version, buffer);
                }
            }
        }
        free(pBlock);
    }
}