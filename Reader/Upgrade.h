#ifndef __UPGRADE_H__
#define __UPGRADE_H__
#ifdef ENABLE_NETWORK

#include <string>
#include <map>
#include "types.h"
#include "https.h"


typedef struct json_item_data_t 
{
    std::wstring version;
    std::wstring url;
    std::wstring desc;
    int force;
} json_item_data_t;

typedef std::map<std::wstring, json_item_data_t*> json_data_t;

typedef BOOL (* upgrade_callback_t)(void *param, json_item_data_t *item);


class Upgrade
{
public:
    Upgrade(void);
    ~Upgrade(void);

public:
    void SetIngoreVersion(TCHAR *ingorev);
    void SetCheckVerTime(u32 *chktime);
    void Check(upgrade_callback_t cb, void *param);
    json_item_data_t * GetVersionInfo(void);

private:
    void CancelRequest(void);
    int ParserJson(const char *json);
    int vercmp(const wchar_t *v1, const wchar_t *v2);

private:
    static unsigned int GetVersionCompleter(request_result_t *result);

    
private:
    TCHAR *m_IngoreVersion;
    u32 *m_ChkTime;
    json_item_data_t m_VersionInfo;
    upgrade_callback_t m_cb;
    void* m_param;
    req_handler_t m_hReq;
};

#endif
#endif
