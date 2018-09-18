#include "StdAfx.h"
#include "Utils.h"
#include <stdint.h>
#include <Wincrypt.h>
#include <string.h>


Utils::Utils(void)
{
}


Utils::~Utils(void)
{
}

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

wchar_t* Utils::ansi_to_unicode(char* str, int* len)
{
	wchar_t* result;
	*len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	result = (wchar_t*)malloc((*len+1)*sizeof(wchar_t));
	memset(result, 0, (*len+1)*sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str, -1, (LPWSTR)result, *len);
	return result;
}

char* Utils::unicode_to_ansi(wchar_t* str, int* len)
{
	char* result;
	*len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	result = (char*)malloc((*len+1)*sizeof(char));
	memset(result, 0, (*len+1)*sizeof(char));
	WideCharToMultiByte(CP_ACP, 0, str, -1, result, *len, NULL, NULL);
	return result;
}

wchar_t* Utils::utf8_to_unicode(char* str, int* len)
{
	wchar_t* result;
	*len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	result = (wchar_t*)malloc((*len+1)*sizeof(wchar_t));
	memset(result, 0, (*len+1)*sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, str, -1, (LPWSTR)result, *len);
	return result;
}

char* Utils::unicode_to_utf8(wchar_t* str, int* len)
{
	char* result;
	*len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
	result =(char*)malloc((*len+1)*sizeof(char));
	memset(result, 0, (*len+1)*sizeof(char));
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