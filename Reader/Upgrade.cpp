#include "StdAfx.h"
#include "Upgrade.h"
#include "cJSON.h"
#include "Utils.h"
#include <process.h>
#include <winhttp.h>

#pragma comment(lib, "Version.lib")
#pragma comment(lib, "winhttp.lib")

#define USER_AGENT      (L"User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.99 Safari/537.36")


Upgrade::Upgrade(void)
    : m_proxy(NULL)
    , m_cb(NULL)
    , m_param(NULL)
    , m_hThread(NULL)
    , m_bForceKill(FALSE)
{
    m_VersionInfo.version.clear();
    m_VersionInfo.url.clear();
    m_VersionInfo.desc.clear();
    m_VersionInfo.force = 0;
}


Upgrade::~Upgrade(void)
{
    CancelRequest();
}

void Upgrade::SetProxy(proxy_t *proxy)
{
    m_proxy = proxy;
}

void Upgrade::SetIngoreVersion(TCHAR *ingorev)
{
    m_IngoreVersion = ingorev;
}

void Upgrade::Check(upgrade_callback_t cb, void *param)
{
    unsigned threadId;

    m_cb = cb;
    m_param = param;
    CancelRequest();
    m_hThread = (HANDLE)_beginthreadex(NULL, 0, DoRequest, this, 0, &threadId);
}

json_item_data_t * Upgrade::GetVersionInfo(void)
{
    return &m_VersionInfo;
}

std::wstring Upgrade::GetApplicationVersion(void)
{
    std::wstring version = L"";
    TCHAR szModPath[MAX_PATH] = {0};
    DWORD dwHandle;
    DWORD dwSize;
    BYTE *pBlock;
    TCHAR SubBlock[MAX_PATH] = {0};
    TCHAR *buffer;
    UINT cbTranslate;

    GetModuleFileName(NULL, szModPath, sizeof(TCHAR)*(MAX_PATH-1));
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
                    version = buffer;
                }
            }
        }
        free(pBlock);
    }

    return version;
}

void Upgrade::CancelRequest(void)
{
    if (m_hThread)
    {
        m_bForceKill = TRUE;
        if (WAIT_TIMEOUT == WaitForSingleObject(m_hThread, 3000))
        {
            TerminateThread(m_hThread, 0);
        }
        if (m_hThread)
        {
            CloseHandle(m_hThread);
            m_hThread = NULL;
        }
    }
}

int Upgrade::ParserJson(const char* json)
{
    cJSON* root = NULL, * list, * item, * version, * url, * desc, * force;
    json_item_data_t data;
    int size,i,len;
    wchar_t* temp;
    std::wstring curversion;

    // parser json
    data.version.clear();
    root = cJSON_Parse(json);
    if (!root)
        goto end;
    list = cJSON_GetObjectItem(root, "list");
    if (!list)
        goto end;
    size = cJSON_GetArraySize(list);
    if (size <= 0)
        goto end;
    for (i = 0; i < size; i++)
    {
        item = cJSON_GetArrayItem(list, i);
        if (!item)
            goto end;
        version = cJSON_GetObjectItem(item, "version");
        if (!version)
            goto end;
        url = cJSON_GetObjectItem(item, "downloadurl");
        if (!url)
            goto end;
        desc = cJSON_GetObjectItem(item, "description");
        if (!desc)
            goto end;
        force = cJSON_GetObjectItem(item, "force");
        if (!force)
            goto end;

        temp = Utils::utf8_to_utf16(version->valuestring, &len);
        if (wcscmp(data.version.c_str(), temp) < 0)
        {
            data.version = temp;
            free(temp);
            temp = Utils::utf8_to_utf16(url->valuestring, &len);
            data.url = temp;
            free(temp);
            temp = Utils::utf8_to_utf16(desc->valuestring, &len);
            data.desc = temp;
            free(temp);
            data.force = force->valueint;
        }
        else
            free(temp);
    }

    if (wcscmp(data.version.c_str(), m_IngoreVersion) == 0)
    {
        // ingore this version
        goto end;
    }

    curversion = GetApplicationVersion();
    if (!curversion.empty())
    {
        if (wcscmp(curversion.c_str(), data.version.c_str()) < 0)
        {
            // backup version in memory
            m_VersionInfo.version = data.version;
            m_VersionInfo.url = data.url;
            m_VersionInfo.desc = data.desc;
            m_VersionInfo.force = data.force;


            // callback
            if (m_cb)
                m_cb(m_param, &data);
        }
    }

end:
    if (root)
        cJSON_Delete(root);
    return 0;
}

unsigned __stdcall Upgrade::DoRequest(void* param)
{
    const WCHAR* szURL = L"https://raw.githubusercontent.com/binbyu/Reader/master/version.json";
    //const WCHAR* szURL = L"https://www.baidu.com";
    Upgrade* _this = (Upgrade*)param;
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    WCHAR szHost[256] = { 0 };
    WCHAR szProxy[256] = { 0 };
    URL_COMPONENTS urlComp = { 0 };
    WINHTTP_PROXY_INFO ProxyInfo = { 0 };
    BOOL bResults = FALSE;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    char *data = NULL;
    std::string json;

    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.lpszHostName = szHost;
    urlComp.dwHostNameLength = sizeof(szHost) / sizeof(szHost[0]);
    urlComp.dwUrlPathLength = (DWORD)-1;
    urlComp.dwSchemeLength = (DWORD)-1;

    if (!WinHttpCrackUrl(szURL, 0, 0, &urlComp))
        goto fail;

    if (_this->m_bForceKill)
        goto fail;

    hSession = WinHttpOpen(USER_AGENT,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession)
        goto fail;

    if (_this->m_bForceKill)
        goto fail;

    hConnect = WinHttpConnect(hSession, szHost,
        urlComp.nPort, 0);
    if (!hConnect)
        goto fail;

    if (_this->m_bForceKill)
        goto fail;

    hRequest = WinHttpOpenRequest(hConnect,
        L"GET", urlComp.lpszUrlPath,
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        INTERNET_SCHEME_HTTPS == urlComp.nScheme ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest)
        goto fail;

    if (_this->m_bForceKill)
        goto fail;

    if (_this->m_proxy && _this->m_proxy->enable)
    {
        swprintf_s(szProxy, L"%s:%d", _this->m_proxy->addr, _this->m_proxy->port);
        ProxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        ProxyInfo.lpszProxy = szProxy;
        ProxyInfo.lpszProxyBypass = (WCHAR*)L"";
        WinHttpSetOption(hRequest,
            WINHTTP_OPTION_PROXY,
            &ProxyInfo,
            sizeof(WINHTTP_PROXY_INFO));

        if (_this->m_bForceKill)
            goto fail;

        WinHttpSetOption(hRequest,
            WINHTTP_OPTION_PROXY_USERNAME,
            (LPVOID)_this->m_proxy->user,
            wcslen(_this->m_proxy->user));

        if (_this->m_bForceKill)
            goto fail;

        WinHttpSetOption(hRequest,
            WINHTTP_OPTION_PROXY_PASSWORD,
            (LPVOID)_this->m_proxy->pass,
            wcslen(_this->m_proxy->pass));

        if (_this->m_bForceKill)
            goto fail;
    }

    // ssl
    DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
        SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);

    if (_this->m_bForceKill)
        goto fail;

    bResults = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0, WINHTTP_NO_REQUEST_DATA, 0,
        0, 0);
    if (!bResults)
        goto fail;

    if (_this->m_bForceKill)
        goto fail;

    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults)
        goto fail;

    if (_this->m_bForceKill)
        goto fail;

    do
    {
        // Check for available data.
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
            goto fail;

        if (_this->m_bForceKill)
            goto fail;

        // No more available data.
        if (!dwSize)
            break;

        // Allocate space for the buffer.
        data = (char *)malloc(dwSize);
        if (!data)
            goto fail;

        // Read the Data.
        if (!WinHttpReadData(hRequest, (LPVOID)data, dwSize, &dwDownloaded))
            goto fail;
        json.append((char*)data, dwSize);

        // Free the memory allocated to the buffer.
        free(data);
        data = NULL;

        if (_this->m_bForceKill)
            goto fail;

    } while (dwSize > 0);


    if (_this->m_bForceKill)
        goto fail;

    _this->ParserJson(json.c_str());

fail:
    if (hRequest)
        WinHttpCloseHandle(hRequest);
    if (hConnect)
        WinHttpCloseHandle(hConnect);
    if (hSession)
        WinHttpCloseHandle(hSession);
    if (data)
        free(data);

    CloseHandle(_this->m_hThread);
    _this->m_hThread = NULL;
    return 0;
}