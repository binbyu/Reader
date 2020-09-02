#ifndef __UPGRADE_H__
#define __UPGRADE_H__

#include <string>
#include <map>
#include "types.h"


typedef struct json_item_data_t 
{
    std::wstring version;
    std::wstring url;
    std::wstring desc;
    int force;
} json_item_data_t;

typedef std::map<std::wstring, json_item_data_t*> json_data_t;

typedef bool (* upgrade_callback_t)(void *param, json_item_data_t *item);


class Upgrade
{
public:
    Upgrade(void);
    ~Upgrade(void);

public:
    void SetProxy(proxy_t *proxy);
    void SetIngoreVersion(TCHAR *ingorev);
    void Check(upgrade_callback_t cb, void *param);
    json_item_data_t * GetVersionInfo(void);
    std::wstring GetApplicationVersion(void);

private:
    void CancelRequest(void);
    int ParserJson(const char *json);
    static unsigned __stdcall DoRequest(void* param);
    int vercmp(const wchar_t *v1, const wchar_t *v2);

    
private:
    TCHAR *m_IngoreVersion;
    json_item_data_t m_VersionInfo;
    proxy_t* m_proxy;
    upgrade_callback_t m_cb;
    void* m_param;
    HANDLE m_hThread;
    BOOL m_bForceKill;
};

#endif
