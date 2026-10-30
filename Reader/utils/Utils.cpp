#include "Utils.h"
#include "conv.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <regex>

bool Utils::isCr(wchar_t c)
{
    return c == 0x000D/* \r CR */;
}

bool Utils::isLf(wchar_t c)
{
    return c == 0x000A/* \n LF */;
}

bool Utils::isCrLf(wchar_t c)
{
    return isCr(c) || isLf(c);
}

bool Utils::isBlank(wchar_t c)
{
    return c == 0x0009/* \t Horizontal Tab */
        || c == 0x0020/* Space */
        || c == 0x000B/* \v Vertical Tab */
        || c == 0x000C/* \f Form Feed */
        || c == 0x00A0/* Non-breaking Space */
        || c == 0x2000/* En Quad */
        || c == 0x2001/* Em Quad */
        || c == 0x2002/* En Space */
        || c == 0x2003/* Em Space */
        || c == 0x2009/* Thin Space */
        || c == 0x200A/* Hair Space */
        || c == 0x3000/* Full-width Space */;
}

bool Utils::isSpace(wchar_t c)
{
    return c == 0x0020/* Half-width Space */
        || c == 0x3000/* Full-width Space */;
}

bool Utils::isHyphen(wchar_t c)
{
    return c == 0x002D/* - Hyphen-minus */
#if 0
        || c == 0x2010/* Hyphen */
        || c == 0x2011/* Non-breaking Hyphen */
        || c == 0x2012/* Figure Dash */
        || c == 0x2013/* En Dash */
        || c == 0x2014/* Em Dash */
        || c == 0x2015/* Horizontal Bar */
        || c == 0x00AD/* Soft Hyphen */
#endif
        ;
}

// [curpos, curpos + len)
// 1. Full text: 123\nabcdef\n456, current at c (text[0]=='c') -> returns: cdef\n (lineLen=4, crlfLen=1)
// 2. Full text: 123\nabcdef\n456, current at a (text[0]=='a') -> returns: abcdef\n (lineLen=6, crlfLen=1)
bool Utils::nextLine(const wchar_t* text, int len, int* lineLen, int* crlfLen, bool* isBlankLine, int* prefixBlankLen, int* suffixBlankLen)
{
    if (lineLen)         *lineLen = 0;
    if (crlfLen)         *crlfLen = 0;
    if (isBlankLine)     *isBlankLine = true;
    if (prefixBlankLen)  *prefixBlankLen = 0;
    if (suffixBlankLen)  *suffixBlankLen = 0;

    if (!text || len <= 0) return false;

    bool hasContent = false;

    for (int i = 0; i < len; i++)
    {
        if (isCr(text[i]))
        {
            if ((i + 1) < len && isLf(text[i + 1]))
            {
                // '\r\n'
                if (lineLen) *lineLen = i;
                if (crlfLen) *crlfLen = 2;
            }
            else
            {
                // '\r'
                if (lineLen) *lineLen = i;
                if (crlfLen) *crlfLen = 1;
            }
            return true;
        }

        if (isLf(text[i]))
        {
            // '\n'
            if (lineLen) *lineLen = i;
            if (crlfLen) *crlfLen = 1;
            return true;
        }

        if (isBlank(text[i]))
        {
            if (!hasContent)
            {
                if (prefixBlankLen) (*prefixBlankLen)++;
            }
            else
            {
                if (suffixBlankLen) (*suffixBlankLen)++;
            }
            continue;
        }

        hasContent = true;
        if (suffixBlankLen) (*suffixBlankLen) = 0;
        if (isBlankLine)    (*isBlankLine) = false;
    }

    if (lineLen) *lineLen = len;
    return true;
}

// [curpos - len, curpos)
// 1. Full text: 123\nabcdef\n456, current at c (text[0]=='c') -> returns: ab (lineLen=2, crlfLen=0)
// 2. Full text: 123\nabcdef\n456, current at 4 (text[0]=='4') -> returns: abcdef\n (lineLen=6, crlfLen=1)
bool Utils::prevLine(const wchar_t* text, int len, int* lineLen, int* crlfLen, bool* isBlankLine, int* prefixBlankLen, int* suffixBlankLen)
{
    if (lineLen)         *lineLen = 0;
    if (crlfLen)         *crlfLen = 0;
    if (isBlankLine)     *isBlankLine = true;
    if (prefixBlankLen)  *prefixBlankLen = 0;
    if (suffixBlankLen)  *suffixBlankLen = 0;

    if (!text || len <= 0) return false;

    bool hasContent = false;

    for (int i = -1; i >= -len; i--)
    {
        if (isLf(text[i]))
        {
            if (i == -1)
            {
                if ((i - 1) >= -len && isCr(text[i - 1]))
                {
                    // '\r\n'
                    if (crlfLen) *crlfLen = 2;
                    i--; // skip '\r'
                }
                else
                {
                    // '\n'
                    if (crlfLen) *crlfLen = 1;
                }
                continue;
            }
            if (lineLen) *lineLen = -i - (*crlfLen) - 1 /* except curpos '\n' */;
            return true;
        }

        if (isCr(text[i]))
        {
            if (i == -1)
            {
                // '\r'
                if (crlfLen) *crlfLen = 1;
                continue;
            }
            if (lineLen) *lineLen = -i - (*crlfLen) - 1 /* except curpos '\r' */;
            return true;
        }

        if (isBlank(text[i]))
        {
            if (!hasContent)
            {
                if (suffixBlankLen) (*suffixBlankLen)++;
            }
            else
            {
                if (prefixBlankLen) (*prefixBlankLen)++;
            }
            continue;
        }

        hasContent = true;
        if (prefixBlankLen) (*prefixBlankLen) = 0;
        if (isBlankLine)    (*isBlankLine) = false;
    }

    if (lineLen) *lineLen = len - (*crlfLen);
    return true;
}

void Utils::formatText(std::wstring& text)
{
    if (text.empty())
        return;

    std::wstring result;
    result.reserve(text.length());

    size_t pos = 0;
    const size_t len = text.length();
    bool isChanged = false;

    while (pos < len)
    {
        size_t lineStart = pos;
        size_t lineEnd = pos;
        while (lineEnd < len && text[lineEnd] != L'\r' && text[lineEnd] != L'\n')
        {
            lineEnd++;
        }

        size_t crlfEnd = lineEnd;
        int crlfLen = 0;
        if (crlfEnd < len && text[crlfEnd] == L'\r')
        {
            crlfLen = 1;
            crlfEnd++;
            if (crlfEnd < len && text[crlfEnd] == L'\n')
            {
                crlfLen = 2;
                crlfEnd++;
            }
        }
        else if (crlfEnd < len && text[crlfEnd] == L'\n')
        {
            crlfLen = 1;
            crlfEnd++;
        }

        size_t prefixLen = 0;
        while (lineStart + prefixLen < lineEnd && isBlank(text[lineStart + prefixLen]))
        {
            prefixLen++;
        }

        size_t suffixLen = 0;
        while (lineEnd - suffixLen > lineStart + prefixLen && isBlank(text[lineEnd - suffixLen - 1]))
        {
            suffixLen++;
        }

        size_t contentLen = (lineEnd - lineStart) - prefixLen - suffixLen;

        bool isBlankLine = (contentLen == 0);

        if (isBlankLine)
        {
            pos = crlfEnd;
            isChanged = true;
            continue;
        }

        if (prefixLen >= 2)
        {
            result.push_back(0x3000);
            result.push_back(0x3000);
            result.append(text, lineStart + prefixLen, contentLen);
            isChanged = true;
        }
        else if (prefixLen == 1 && text[lineStart] != L'\t')
        {
            result.push_back(L'\t');
            result.append(text, lineStart + prefixLen, contentLen);
            isChanged = true;
        }
        else
        {
            result.append(text, lineStart, contentLen + prefixLen);
            if (suffixLen > 0)
            {
                isChanged = true;
            }
        }

        if (crlfLen > 0)
        {
            result.push_back(L'\n');
            if (crlfLen != 1 || text[lineEnd] != L'\n')
            {
                isChanged = true;
            }
        }

        pos = crlfEnd;
    }

    if (isChanged)
    {
        text = std::move(result);
    }
}

std::wstring Utils::fileExt(const std::wstring& path)
{
#if 0
    std::wstring ext = path.substr(path.find_last_of(L".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
#else
    return PathFindExtensionW(path.c_str());
#endif
}

std::wstring Utils::fileName(const std::wstring& path)
{
    wchar_t szFileName[MAX_PATH] = { 0 };
    wcscpy(szFileName, path.c_str());
    PathRemoveExtensionW(szFileName);
    return PathFindFileNameW(szFileName);
}

bool Utils::isFileExist(const std::wstring& path)
{
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }
    return true;
}

bool Utils::isFileEmpty(const std::wstring& path)
{
    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER fileSize;
    BOOL result = GetFileSizeEx(hFile, &fileSize);
    CloseHandle(hFile);

    if (!result)
        return false;
    return (fileSize.QuadPart == 0);
}

bool Utils::removeFile(const std::wstring& path)
{
    if (DeleteFileW(path.c_str()))
    {
        return true;
    }
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
    {
        return true;
    }
    return false;
}

bool Utils::makeDir(const std::wstring& path)
{
    if (path.empty())
        return false;

    std::wstring cleanPath = path;
    while (!cleanPath.empty() && (cleanPath.back() == L'\\' || cleanPath.back() == L'/'))
    {
        if (cleanPath.size() == 3 && cleanPath[1] == L':' && (cleanPath[2] == L'\\' || cleanPath[2] == L'/'))
            break;
        if (cleanPath.size() == 2 && cleanPath[1] == L':')
            break;
        cleanPath.pop_back();
    }

    if (cleanPath.empty())
        return false;

    if (::CreateDirectoryW(cleanPath.c_str(), NULL))
        return true;

    DWORD err = ::GetLastError();
    if (err == ERROR_ALREADY_EXISTS)
        return true;

    if (err != ERROR_PATH_NOT_FOUND)
        return false;

    size_t pos = cleanPath.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
        return false;

    std::wstring parent = cleanPath.substr(0, pos);
    if (parent.size() == 2 && parent[1] == L':')
        parent += L'\\';

    if (!makeDir(parent))
        return false;

    return (::CreateDirectoryW(cleanPath.c_str(), NULL) || ::GetLastError() == ERROR_ALREADY_EXISTS);
}

bool Utils::renameFile(const std::wstring& src, const std::wstring& dst)
{
    return MoveFileExW(src.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
}

std::wstring Utils::appPath()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring fullPath(buffer);
    return fullPath.substr(0, fullPath.find_last_of(L"\\/") + 1);
}

std::wstring Utils::appVersion()
{
    wchar_t szModPath[MAX_PATH] = { 0 };
    DWORD dwHandle;
    DWORD dwSize;
    BYTE* pBlock;
    wchar_t SubBlock[MAX_PATH] = { 0 };
    wchar_t* buffer;
    UINT cbTranslate;
    std::wstring version;

    GetModuleFileNameW(NULL, szModPath, sizeof(wchar_t) * (MAX_PATH - 1));
    dwSize = GetFileVersionInfoSizeW(szModPath, &dwHandle);
    if (dwSize > 0)
    {
        pBlock = (BYTE*)malloc(dwSize);
        memset(pBlock, 0, dwSize);
        if (GetFileVersionInfoW(szModPath, dwHandle, dwSize, pBlock))
        {
            struct LANGANDCODEPAGE {
                WORD wLanguage;
                WORD wCodePage;
            } *lpTranslate;

            // Read the list of languages and code pages.
            if (VerQueryValueW(pBlock, L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate))
            {
                // Read the file description for each language and code page.
                wsprintfW(SubBlock, L"\\StringFileInfo\\%04x%04x\\FileVersion", lpTranslate->wLanguage, lpTranslate->wCodePage);
                if (VerQueryValueW(pBlock, SubBlock, (LPVOID*)&buffer, &cbTranslate))
                {
                    version = buffer;
                }
            }
        }
        free(pBlock);
    }
    return version;
}

std::wstring Utils::dirtyFileName(const std::wstring& fileName)
{
    std::wregex re(L"[\\\\/:*?\"<>|]");
    std::wstring fname = trim(fileName);
    return std::regex_replace(fname, re, L"_");
}

long long Utils::nowMs()
{
    FILETIME ft;
    long long t;

    GetSystemTimeAsFileTime(&ft);
    t = ((long long)ft.dwHighDateTime << 32) | (long long)ft.dwLowDateTime;
    return t / 10000 - 11644473600000LL;    // 100ns -> ms, 1601 -> 1970
}

std::wstring Utils::timeStr(std::time_t t)
{
    std::tm* localTime = std::localtime(&t);
    std::wstringstream wss;
    wss << std::put_time(localTime, L"%Y-%m-%d %H:%M:%S");
    return wss.str();
}

std::wstring Utils::currentDateTime()
{
    return timeStr(std::time(nullptr));
}

std::wstring Utils::currentYear()
{
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    std::wstringstream wss;
    wss << std::put_time(localTime, L"%Y");
    return wss.str();
}

std::wstring Utils::lastErrorMessage(DWORD errorCode)
{
    if (errorCode == 0)
    {
        return L"No error";
    }
    std::wstring message;
    LPWSTR buffer = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buffer, 0, nullptr);
    if (buffer)
    {
        message = buffer;
        LocalFree(buffer);
    }
    return message;
}

void Utils::replace(std::wstring& str, const std::wstring& from, const std::wstring& to)
{
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::wstring::npos)
    {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
}

void Utils::replace(std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::wstring::npos)
    {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
}

std::vector<std::wstring> Utils::split(std::wstring& str, const std::wstring& delimiter, bool keepEmpty)
{
    std::vector<std::wstring> tokens;
    if (str.empty()) return tokens;
    size_t start = 0;
    size_t end = 0;
    while ((end = str.find(delimiter, start)) != std::string::npos)
    {
        std::wstring token = str.substr(start, end - start);
        if (keepEmpty || !token.empty())
        {
            tokens.push_back(token);
        }
        start = end + delimiter.length();
    }
    std::wstring lastToken = str.substr(start);
    if (keepEmpty || !lastToken.empty())
    {
        tokens.push_back(lastToken);
    }
    return tokens;
}

std::vector<std::string> Utils::split(std::string& str, const std::string& delimiter, bool keepEmpty)
{
    std::vector<std::string> tokens;
    if (str.empty()) return tokens;
    size_t start = 0;
    size_t end = 0;
    while ((end = str.find(delimiter, start)) != std::string::npos)
    {
        std::string token = str.substr(start, end - start);
        if (keepEmpty || !token.empty())
        {
            tokens.push_back(token);
        }
        start = end + delimiter.length();
    }
    std::string lastToken = str.substr(start);
    if (keepEmpty || !lastToken.empty())
    {
        tokens.push_back(lastToken);
    }
    return tokens;
}

std::wstring Utils::trimLeft(const std::wstring& str)
{
    size_t start = 0;
    while (start < str.size() && (isBlank(str[start]) || isCrLf(str[start])))
        ++start;
    return str.substr(start);
}

std::wstring Utils::trimRight(const std::wstring& str)
{
    if (str.empty()) return str;
    size_t end = str.size() - 1;
    while (end > 0 && (isBlank(str[end]) || isCrLf(str[end])))
        --end;
    return str.substr(0, (isBlank(str[end]) || isCrLf(str[end])) ? 0 : end + 1);
}

std::wstring Utils::trim(const std::wstring& str)
{
    return trimLeft(trimRight(str));
}

std::string Utils::trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return std::string();
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::wstring Utils::toRGB(COLORREF color)
{
    wchar_t rgb[8] = { 0 };
    _snwprintf(rgb, 8, L"#%02X%02X%02X", GetRValue(color), GetGValue(color), GetBValue(color));
    return rgb;
}

std::wstring Utils::fnv1a32(const void* data, size_t size)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < size; i++)
    {
        h ^= p[i];
        h *= 16777619u;
    }

    wchar_t hash[16] = {0};
    swprintf(hash, MAX_PATH, L"%08x", h);
    return hash;
}

#ifdef ENABLE_NETWORK
std::vector<AdapterInfo> Utils::enumNetworkAdapters()
{
    std::vector<AdapterInfo> adapters;

    ULONG bufSize = 0;
    GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, nullptr, &bufSize);
    
    std::vector<BYTE> buf(bufSize);
    PIP_ADAPTER_ADDRESSES pAdapter = (PIP_ADAPTER_ADDRESSES)buf.data();
    
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, pAdapter, &bufSize) != ERROR_SUCCESS)
        return adapters;

    while (pAdapter)
    {
        if (pAdapter->OperStatus == IfOperStatusUp)
        {
            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pAdapter->FirstUnicastAddress;
            while (pUnicast)
            {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
                {
                    sockaddr_in* sin = (sockaddr_in*)pUnicast->Address.lpSockaddr;
#if _USING_V110_SDK71_
                    char* ip = inet_ntoa(sin->sin_addr);
#else
                    char ip[16];
                    inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip));
#endif
                    
                    if (strncmp(ip, "127.", 4) != 0)
                    {
                        AdapterInfo info;
                        info.name = pAdapter->FriendlyName ? pAdapter->FriendlyName : L"";
                        info.ip = ip;
                        adapters.push_back(info);
                    }
                }
                pUnicast = pUnicast->Next;
            }
        }
        pAdapter = pAdapter->Next;
    }
    return adapters;
}

std::string Utils::adapterIp(const std::wstring& adapterName)
{
    if (adapterName.empty())
    {
        return "";
    }

    std::string ip;
    auto adapters = enumNetworkAdapters();
    for (const auto& adapter : adapters)
    {
        if (adapter.name == adapterName)
        {
            ip = adapter.ip;
            break;
        }
    }
    return ip;
}

bool Utils::isValidHref(const std::string& href)
{
    if (href.empty())
        return false;
    if (href[0] == '#')
        return false;

    std::string lowerHref = href;
    std::transform(lowerHref.begin(), lowerHref.end(), lowerHref.begin(), ::tolower);

    if (lowerHref.find("mailto:") == 0)
        return false;
    if (lowerHref.find("tel:") == 0)
        return false;
    if (lowerHref.find("javascript:") == 0)
        return false;
    size_t pos = lowerHref.find("://");
    if (pos != std::string::npos)
    {
        std::string protocol = lowerHref.substr(0, pos);
        if (!protocol.empty() && protocol != "http" && protocol != "https")
            return false;
    }
    return true;
}

void Utils::dumpHtmlParseErrorFile(const char *html, int htmlLen)
{
#ifdef _DEBUG
    const char *UTF_8_BOM = "\xEF\xBB\xBF";
    FILE *fp;
    if (html && htmlLen > 0)
    {
        fp = _wfopen(L"dump.html", L"wb");
        if (fp)
        {
            fwrite(UTF_8_BOM, 1, 3, fp);
            fwrite(html, 1, htmlLen, fp);
            fclose(fp);
        }
    }
#endif
}

void Utils::dumpJsonParseErrorFile(const char *json, int jsonLen)
{
#ifdef _DEBUG
    const char *UTF_8_BOM = "\xEF\xBB\xBF";
    FILE *fp;
    if (json && jsonLen > 0)
    {
        fp = _wfopen(L"dump.json", L"wb");
        if (fp)
        {
            fwrite(UTF_8_BOM, 1, 3, fp);
            fwrite(json, 1, jsonLen, fp);
            fclose(fp);
        }
    }
#endif
}
#endif