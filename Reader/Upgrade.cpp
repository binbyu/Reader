#ifdef ENABLE_NETWORK
#include "Upgrade.h"
#include "cJSON.h"
#include "Utils.h"
#include "HtmlParser.h"
#include <process.h>
#include <time.h>


Upgrade::Upgrade(void)
    : m_IngoreVersion(NULL)
    , m_ChkTime(NULL)
    , m_cb(NULL)
    , m_param(NULL)
    , m_hReq(NULL)
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

void Upgrade::SetIngoreVersion(TCHAR *ingorev)
{
    m_IngoreVersion = ingorev;
}

void Upgrade::SetCheckVerTime(u32 *chktime)
{
    m_ChkTime = chktime;
}

void Upgrade::Check(upgrade_callback_t cb, void *param)
{
    const char* url = "https://raw.githubusercontent.com/binbyu/Reader/master/version.json";
    request_t req;
    u32 curtime;

    // last check version time
    if (m_ChkTime)
    {
        curtime = _time32(NULL);
        if (curtime - (*m_ChkTime) < 3600 * 12)
        {
            return;
        }
    }

    m_cb = cb;
    m_param = param;
    CancelRequest();

    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = (char *)url;
    req.completer = GetVersionCompleter;
    req.param1 = this;
    req.param2 = NULL;

    m_hReq = hapi_request(&req);
}

json_item_data_t * Upgrade::GetVersionInfo(void)
{
    return &m_VersionInfo;
}

void Upgrade::CancelRequest(void)
{
    if (m_hReq)
        hapi_cancel(m_hReq);
    m_hReq = NULL;
}

int Upgrade::ParserJson(const char* json)
{
    cJSON* root = NULL, * list, * item, * version, * url, * desc, * force;
    json_item_data_t data;
    int size,i;
    wchar_t* temp;
    std::wstring curversion;
    TCHAR appver[256] = { 0 };

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

        temp = Utf8ToUtf16(version->valuestring);
        if (vercmp(data.version.c_str(), temp) < 0)
        {
            data.version = temp;
            temp = Utf8ToUtf16(url->valuestring);
            data.url = temp;
            temp = Utf8ToUtf16(desc->valuestring);
            data.desc = temp;
            data.force = force->valueint;
        }
    }

    if (vercmp(data.version.c_str(), m_IngoreVersion) == 0)
    {
        // ingore this version
        goto end;
    }

    GetApplicationVersion(appver);
    curversion = appver;
    if (!curversion.empty())
    {
        if (vercmp(curversion.c_str(), data.version.c_str()) < 0)
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

int Upgrade::vercmp(const wchar_t *v1, const wchar_t *v2)
{
    unsigned int v1_1 = 0, v1_2 = 0, v1_3 = 0, v1_4 = 0;
    unsigned int v2_1 = 0, v2_2 = 0, v2_3 = 0, v2_4 = 0;
    swscanf(v1, L"%d.%d.%d.%d", &v1_1, &v1_2, &v1_3, &v1_4);
    swscanf(v2, L"%d.%d.%d.%d", &v2_1, &v2_2, &v2_3, &v2_4);

    unsigned int num1 = v1_1 << 24 | v1_2 << 16 | v1_3 << 8 | v1_4;
    unsigned int num2 = v2_1 << 24 | v2_2 << 16 | v2_3 << 8 | v2_4;

    return num1 == num2 ? 0 : (num1 > num2 ? 1 : -1);
}

unsigned int Upgrade::GetVersionCompleter(request_result_t *result)
{
    Upgrade *_this = (Upgrade *)result->param1;

    _this->m_hReq = NULL;
    *_this->m_ChkTime = _time32(NULL);

    if (result->cancel)
        return 1;

    if (result->errno_ != succ)
        return 1;

    if (result->status_code != 200)
    {
        return 1;
    }

    if (result->body && result->bodylen > 0)
    {
        _this->ParserJson(result->body);
    }
    return 0;
}

#endif
