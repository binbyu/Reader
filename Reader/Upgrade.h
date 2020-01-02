#ifndef __UPGRADE_H__
#define __UPGRADE_H__

#include <string>
#include <map>
#include "HttpClient.h"


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

private:
    std::wstring GetApplicationVersion(void);

private:
    static unsigned int GetVersionListCompleter(bool result, request_t *req);
    static unsigned int writer_string(void *data, unsigned int size, unsigned int nmemb, void *stream);
    
private:
    HttpClient m_HttpClient;
    TCHAR *m_IngoreVersion;
    json_item_data_t m_VersionInfo;
};

#endif
