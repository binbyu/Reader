#include "Utils.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#ifdef ZLIB_ENABLE
#include "zlib.h"
#else
#include "miniz.h"
#endif

const char* UTF_16_BE_BOM = "\xFE\xFF";
const char* UTF_16_LE_BOM = "\xFF\xFE";
const char* UTF_8_BOM = "\xEF\xBB\xBF";
const char* UTF_32_BE_BOM = "\x00\x00\xFE\xFF";
const char* UTF_32_LE_BOM = "\xFF\xFE\x00\x00";

static char* _result = NULL;
static int _len = 0;
static wchar_t* _wresult = NULL;
static int _wlen = 0;

wchar_t* ansi_to_utf16(const char* str, int size, int* len)
{
    wchar_t* result;
    *len = MultiByteToWideChar(CP_ACP, 0, str, size, NULL, 0);
    result = (wchar_t*)malloc(((*len)+1)*sizeof(wchar_t));
    result[*len] = 0;
    MultiByteToWideChar(CP_ACP, 0, str, size, (LPWSTR)result, *len);
    return result;
}

char* utf16_to_ansi(const wchar_t* str, int size, int* len)
{
    char* result;
    *len = WideCharToMultiByte(CP_ACP, 0, str, size, NULL, 0, NULL, NULL);
    result = (char*)malloc(((*len)+1) * sizeof(char));
    result[*len] = 0;
    WideCharToMultiByte(CP_ACP, 0, str, size, result, *len, NULL, NULL);
    return result;
}

wchar_t* utf8_to_utf16(const char* str, int size, int* len)
{
    wchar_t* result;
    *len = MultiByteToWideChar(CP_UTF8, 0, str, size, NULL, 0);
    result = (wchar_t*)malloc(((*len)+1) * sizeof(wchar_t));
    result[*len] = 0;
    MultiByteToWideChar(CP_UTF8, 0, str, size, (LPWSTR)result, *len);
    return result;
}

char* utf16_to_utf8(const wchar_t* str, int size, int* len)
{
    char* result;
    *len = WideCharToMultiByte(CP_UTF8, 0, str, size, NULL, 0, NULL, NULL);
    result = (char*)malloc(((*len)+1) * sizeof(char));
    result[*len] = 0;
    WideCharToMultiByte(CP_UTF8, 0, str, size, result, *len, NULL, NULL);
    return result;
}

char* utf16_to_utf8_bom(const wchar_t* str, int size, int* len)
{
    char* result;
    int bom_header_len = 3;//strlen(UTF_8_BOM);
    *len = WideCharToMultiByte(CP_UTF8, 0, str, size, NULL, 0, NULL, NULL);
    result = (char*)malloc(((*len) + 1) * sizeof(char) + bom_header_len);
    result[*len + bom_header_len] = 0;
    memcpy(result, UTF_8_BOM, bom_header_len);
    WideCharToMultiByte(CP_UTF8, 0, str, size, result + bom_header_len, *len, NULL, NULL);
    return result;
}

void free_buffer(void* buffer)
{
    if (buffer)
    {
        free(buffer);
    }
}

char* Utf16ToUtf8(const wchar_t* str)
{
    int len;

    len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    if (!_result)
    {
        _len = len + 1;
        _result = (char*)malloc(_len * sizeof(char));
    }
    else if (_len < len + 1)
    {
        _len = len + 1;
        _result = (char*)realloc(_result, _len * sizeof(char));
    }

    WideCharToMultiByte(CP_UTF8, 0, str, -1, _result, len, NULL, NULL);
    _result[len] = 0;
    return _result;
}

char *Utf16ToAnsi(const wchar_t* str)
{
    int len;

    len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    if (!_result)
    {
        _len = len + 1;
        _result = (char*)malloc(_len * sizeof(char));
    }
    else if (_len < len + 1)
    {
        _len = len + 1;
        _result = (char*)realloc(_result, _len * sizeof(char));
    }

    WideCharToMultiByte(CP_ACP, 0, str, -1, _result, len, NULL, NULL);
    _result[len] = 0;
    return _result;
}

wchar_t* Utf8ToUtf16(const char* str)
{
    int len;

    len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (!_wresult)
    {
        _wlen = len + 1;
        _wresult = (wchar_t*)malloc(_wlen * sizeof(wchar_t));
    }
    else if (_wlen < len + 1)
    {
        _wlen = len + 1;
        _wresult = (wchar_t*)realloc(_wresult, _wlen * sizeof(wchar_t));
    }

    MultiByteToWideChar(CP_UTF8, 0, str, -1, (LPWSTR)_wresult, len);
    _wresult[len] = 0;
    return _wresult;
}

void FreeConvertBuffer()
{
    if (_result)
    {
        free(_result);
        _result = NULL;
    }
    if (_wresult)
    {
        free(_wresult);
        _wresult = NULL;
    }
    _len = 0;
    _wlen = 0;
}

type_t check_bom(const char *data, size_t size)
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

int is_ascii(const char *data, size_t size)
{
    const unsigned char *str = (const unsigned char*)data;
    const unsigned char *end = str + size;
    for (; str != end; str++) {
        if (*str & 0x80)
            return 0;
    }
    return 1;
}

int is_utf8(const char *data, size_t size)
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

char* le_to_be(char* data, int len)
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

char* be_to_le(char* data, int len)
{
    return le_to_be(data, len);
}

void b64_encode(const char *src, int slen, char *dst, int *dlen)
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

void b64_decode(const char *src, int slen, char *dst, int *dlen)
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

static char to_hex(char code) {
    static char hex[] = "0123456789abcdef";
    return hex[code & 15];
}

static char from_hex(char ch) {
    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/*!
size of dest must have min size of  3*strlen(src) +1
*/
int url_encode(const char *src, char *dest)
{
    const char *pstr = src;
    char *pbuf = dest;

    if (src == NULL || dest == NULL)
    {
        return -1;
    }
    while (*pstr)
    {
        if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
            *pbuf++ = *pstr;
        else if (*pstr == ' ')
            *pbuf++ = '+';
        else
            *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
        pstr++;
    }
    *pbuf = '\0';
    return (int)(pbuf - dest);
}

/*!
size of dest must have min size of  strlen(src) +1
*/
int url_decode(const char *src, char *dest)
{
    const char *pstr = src;
    char *pbuf = dest;

    if (src == NULL || dest == NULL)
    {
        return -1;
    }
    while (*pstr)
    {
        if (*pstr == '%')
        {
            if (pstr[1] && pstr[2])
            {
                *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
                pstr += 2;
            }
        }
        else if (*pstr == '+')
        {
            *pbuf++ = ' ';
        }
        else
        {
            *pbuf++ = *pstr;
        }
        pstr++;
    }
    *pbuf = '\0';
    return (int)(pbuf - dest);
}

static const unsigned char _charmap[] = {
    u'\000', u'\001', u'\002', u'\003', u'\004', u'\005', u'\006', u'\007',
    u'\010', u'\011', u'\012', u'\013', u'\014', u'\015', u'\016', u'\017',
    u'\020', u'\021', u'\022', u'\023', u'\024', u'\025', u'\026', u'\027',
    u'\030', u'\031', u'\032', u'\033', u'\034', u'\035', u'\036', u'\037',
    u'\040', u'\041', u'\042', u'\043', u'\044', u'\045', u'\046', u'\047',
    u'\050', u'\051', u'\052', u'\053', u'\054', u'\055', u'\056', u'\057',
    u'\060', u'\061', u'\062', u'\063', u'\064', u'\065', u'\066', u'\067',
    u'\070', u'\071', u'\072', u'\073', u'\074', u'\075', u'\076', u'\077',
    u'\100', u'\141', u'\142', u'\143', u'\144', u'\145', u'\146', u'\147',
    u'\150', u'\151', u'\152', u'\153', u'\154', u'\155', u'\156', u'\157',
    u'\160', u'\161', u'\162', u'\163', u'\164', u'\165', u'\166', u'\167',
    u'\170', u'\171', u'\172', u'\133', u'\134', u'\135', u'\136', u'\137',
    u'\140', u'\141', u'\142', u'\143', u'\144', u'\145', u'\146', u'\147',
    u'\150', u'\151', u'\152', u'\153', u'\154', u'\155', u'\156', u'\157',
    u'\160', u'\161', u'\162', u'\163', u'\164', u'\165', u'\166', u'\167',
    u'\170', u'\171', u'\172', u'\173', u'\174', u'\175', u'\176', u'\177',
    u'\200', u'\201', u'\202', u'\203', u'\204', u'\205', u'\206', u'\207',
    u'\210', u'\211', u'\212', u'\213', u'\214', u'\215', u'\216', u'\217',
    u'\220', u'\221', u'\222', u'\223', u'\224', u'\225', u'\226', u'\227',
    u'\230', u'\231', u'\232', u'\233', u'\234', u'\235', u'\236', u'\237',
    u'\240', u'\241', u'\242', u'\243', u'\244', u'\245', u'\246', u'\247',
    u'\250', u'\251', u'\252', u'\253', u'\254', u'\255', u'\256', u'\257',
    u'\260', u'\261', u'\262', u'\263', u'\264', u'\265', u'\266', u'\267',
    u'\270', u'\271', u'\272', u'\273', u'\274', u'\275', u'\276', u'\277',
    u'\300', u'\341', u'\342', u'\343', u'\344', u'\345', u'\346', u'\347',
    u'\350', u'\351', u'\352', u'\353', u'\354', u'\355', u'\356', u'\357',
    u'\360', u'\361', u'\362', u'\363', u'\364', u'\365', u'\366', u'\367',
    u'\370', u'\371', u'\372', u'\333', u'\334', u'\335', u'\336', u'\337',
    u'\340', u'\341', u'\342', u'\343', u'\344', u'\345', u'\346', u'\347',
    u'\350', u'\351', u'\352', u'\353', u'\354', u'\355', u'\356', u'\357',
    u'\360', u'\361', u'\362', u'\363', u'\364', u'\365', u'\366', u'\367',
    u'\370', u'\371', u'\372', u'\373', u'\374', u'\375', u'\376', u'\377',
};

int strcasecmp(const char *s1, const char *s2)
{
    const unsigned char *cm = _charmap;
    const unsigned char *us1 = (const unsigned char *)s1;
    const unsigned char *us2 = (const unsigned char *)s2;

    while (cm[*us1] == cm[*us2++])
        if (*us1++ == '\0')
            return (0);
    return (cm[*us1] - cm[*--us2]);
}

int strncasecmp(const char *s1, const char *s2, int n)
{
    if (n != 0) {
        const unsigned char *cm = _charmap;
        const unsigned char *us1 = (const unsigned char *)s1;
        const unsigned char *us2 = (const unsigned char *)s2;

        do {
            if (cm[*us1] != cm[*us2++])
                return (cm[*us1] - cm[*--us2]);
            if (*us1++ == '\0')
                break;
        } while (--n != 0);
    }
    return (0);
}

char *strcasestr(const char *s, const char *find)
{
    char c, sc;
    int len;

    if ((c = *find++) != 0) {
        c = (char)tolower((unsigned char)c);
        len = (int)strlen(find);
        do {
            do {
                if ((sc = *s++) == 0)
                    return (NULL);
            } while ((char)tolower((unsigned char)sc) != c);
        } while (strncasecmp(s, find, len) != 0);
        s--;
    }
    return ((char *)s);
}

BOOL Is_WinXP_SP2_or_Later(void)
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

void GetApplicationVersion(TCHAR* version)
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

// return: 1, TRUE, 0, FALSE
int memvcmp(void *memory, unsigned char val, unsigned int size)
{
    unsigned char *mm = (unsigned char*)memory;
    return (*mm == val) && memcmp(mm, mm + 1, size - 1) == 0;
}
