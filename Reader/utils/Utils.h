#pragma once
#include <string>
#include <vector>
#include <ctime>
#include "framework.h"
#include "common.h"

#ifdef ENABLE_NETWORK
struct AdapterInfo
{
    std::wstring name;
    // std::wstring description;
    // std::string mac;
    std::string ip;
    // std::string gateway;
    // std::string subnet;
};
#endif

class Utils
{
public:
    static bool isCr(wchar_t c);
    static bool isLf(wchar_t c);
    static bool isCrLf(wchar_t c);
    static bool isBlank(wchar_t c);
    static bool isSpace(wchar_t c);
    static bool isHyphen(wchar_t c);
    static bool nextLine(const wchar_t* text, int len, int* lineLen, int* crlfLen, bool* isBlankLine, int* prefixBlankLen, int* suffixBlankLen);
    static bool prevLine(const wchar_t* text, int len, int* lineLen, int* crlfLen, bool* isBlankLine, int* prefixBlankLen, int* suffixBlankLen);
    static void formatText(std::wstring& text);
public:
    static std::wstring fileExt(const std::wstring& path);
    static std::wstring fileName(const std::wstring& path);
    static bool isFileExist(const std::wstring& path);
    static bool isFileEmpty(const std::wstring& path);
    static bool removeFile(const std::wstring& path);
    static bool makeDir(const std::wstring& path);
    static bool renameFile(const std::wstring& src, const std::wstring& dst);
    static std::wstring appPath();
    static std::wstring appVersion();
    static std::wstring dirtyFileName(const std::wstring& fileName);
public:
    static long long nowMs();
    static std::wstring timeStr(std::time_t t);
    static std::wstring currentDateTime();
    static std::wstring currentYear();
    static std::wstring lastErrorMessage(DWORD errorCode);
    static void replace(std::wstring& str, const std::wstring& from, const std::wstring& to);
    static void replace(std::string& str, const std::string& from, const std::string& to);
    static std::vector<std::wstring> split(std::wstring& str, const std::wstring& delimiter, bool keepEmpty = true);
    static std::vector<std::string> split(std::string& str, const std::string& delimiter, bool keepEmpty = true);
    static std::wstring trimLeft(const std::wstring& str);
    static std::wstring trimRight(const std::wstring& str);
    static std::wstring trim(const std::wstring& str);
    static std::string trim(const std::string& str);
    static std::wstring toRGB(COLORREF color);
    static std::wstring fnv1a32(const void* data, size_t size);
public:
#ifdef ENABLE_NETWORK
    static std::vector<AdapterInfo> enumNetworkAdapters();
    static std::string adapterIp(const std::wstring& adapterName);
    static bool isValidHref(const std::string& href);
    static void dumpHtmlParseErrorFile(const char *html, int htmlLen);
    static void dumpJsonParseErrorFile(const char *json, int jsonLen);
#endif
};
