#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "NetEvent.h"
#include "types.h"
#include <string>
#include <queue>

struct request_t;
typedef unsigned int(*progress_cb)(void *param, double dltotal, double dlnow, double ultotal, double ulnow);
typedef unsigned int(*write_data_cb)(void *data, unsigned int size, unsigned int nmemb, void *stream);
typedef unsigned int(*complete_cb)(bool result, request_t *req);


typedef enum request_method_t
{
    GET,
    POST
} request_method_t;

typedef struct request_t
{
    request_method_t method;
    std::string url;
    std::string json; // just for POST
    std::string cookie;
    write_data_cb writer;
    void *stream; // for writer
    progress_cb progresser;
    void *param; // for progresser
    complete_cb completer;
    void *arg; // for completer
    void *param1; // for all
    void *param2; // for all

    request_t()
    {
        this->method = GET;
        this->url = "";
        this->json = "";
        this->cookie = "";
        this->writer = NULL;
        this->stream = NULL;
        this->progresser = NULL;
        this->param = NULL;
        this->completer = NULL;
        this->arg = NULL;
        this->param1 = NULL;
        this->param2 = NULL;
    }

    request_t(const request_t &r)
    {
        this->method = r.method;
        this->url = r.url;
        this->json = r.json;
        this->cookie = r.cookie;
        this->writer = r.writer;
        this->stream = r.stream;
        this->progresser = r.progresser;
        this->param = r.param;
        this->completer = r.completer;
        this->arg = r.arg;
        this->param1 = r.param1;
        this->param2 = r.param2;
    }
} request_t;

typedef std::queue<request_t *> request_queue_t;

typedef enum http_errno_t
{
    succ = 0,
    fail = -1,
    comp = -2,
    clos = -3,
    nonb = -4,
} http_errno_t;

class HttpClient
{
public:
    HttpClient();
    ~HttpClient();

    int SetProxy(proxy_t *proxy);
    int Request(request_t &req);

private:
    int OnRequest(request_t *req);
    int ParseUrl(const std::string &url, std::string &host, std::string &path, int &port);
    int GetAddr(const std::string &host, in_addr *addr);
    int ConnectServer(event_t *ev);
    int ConnectProxyReq(event_t *ev);
    int ConnectProxyRsp(event_t *ev);
    int SendRequest(event_t *ev);
    int RecvHeader(event_t *ev);
    int RecvBody(event_t *ev);
    int ReadChunkedSize(SOCKET fd, int &size);

private:
    static unsigned __stdcall EventThread(void* arg);
    static void EventHandler(event_t *ev);

private:
    proxy_t *m_Proxy;
    NetEvent m_Event;
    HANDLE m_hTread;
    request_queue_t m_Queue;
    HANDLE m_hMutex;
};

#endif // !__HTTP_CLIENT_H__
