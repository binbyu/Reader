#include "StdAfx.h"
#include "Upgrade.h"
#include "cJSON.h"
#include "Utils.h"

#pragma comment(lib, "Version.lib")


Upgrade::Upgrade(void)
{
    m_VersionInfo.version.clear();
    m_VersionInfo.url.clear();
    m_VersionInfo.desc.clear();
    m_VersionInfo.force = 0;
}


Upgrade::~Upgrade(void)
{
}

void Upgrade::SetProxy(proxy_t *proxy)
{
    m_HttpClient.SetProxy(proxy);
}

void Upgrade::SetIngoreVersion(TCHAR *ingorev)
{
    m_IngoreVersion = ingorev;
}

void Upgrade::Check(upgrade_callback_t cb, void *param)
{
    request_t req;
    req.method = GET;
    req.url = "https://raw.githubusercontent.com/binbyu/Reader/master/version.json";
    req.writer = writer_string;
    req.stream = new std::string;
    req.completer = GetVersionListCompleter;
    req.arg = cb;
    req.param1 = param;
    req.param2 = this;

    m_HttpClient.Request(req);
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

unsigned int Upgrade::GetVersionListCompleter(bool result, request_t *req)
{
    Upgrade *_this = (Upgrade *)req->param2;
    upgrade_callback_t cb = (upgrade_callback_t)req->arg;
    std::string *json = (std::string *)req->stream;
    int len;
    wchar_t *temp;
    cJSON *root = NULL, *list, *item, *version, *url, *desc, *force;
    int i, size;
    json_item_data_t data;
    std::wstring curversion;

    if (!result)
        goto end;

    // parser json
    data.version.clear();
    root = cJSON_Parse(json->c_str());
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

        temp = Utils::utf8_to_unicode(version->valuestring, &len);
        if (wcscmp(data.version.c_str(), temp) < 0)
        {
            data.version = temp;
            free(temp);
            temp = Utils::utf8_to_unicode(url->valuestring, &len);
            data.url = temp;
            free(temp);
            temp = Utils::utf8_to_unicode(desc->valuestring, &len);
            data.desc = temp;
            free(temp);
            data.force = force->valueint;
        }
    }

    if (wcscmp(data.version.c_str(), _this->m_IngoreVersion) == 0)
    {
        // ingore this version
        goto end;
    }
    
    curversion = _this->GetApplicationVersion();
    if (!curversion.empty())
    {
        if (wcscmp(curversion.c_str(), data.version.c_str()) < 0)
        {
            // backup version in memory
            _this->m_VersionInfo.version = data.version;
            _this->m_VersionInfo.url = data.url;
            _this->m_VersionInfo.desc = data.desc;
            _this->m_VersionInfo.force = data.force;


            // callback
            if (cb)
                cb(req->param1, &data);
        }
    }

end:
    if (root)
        cJSON_Delete(root);
    delete json;
    return 0;
}

unsigned int Upgrade::writer_string(void *data, unsigned int size, unsigned int nmemb, void *stream)
{
    std::string *html = (std::string *)stream;
    html->append((char*)data, size * nmemb);
    return size * nmemb;
}