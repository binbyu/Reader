#pragma once
#include "https.h"
#include "common.h"
#include "IWnd.h"
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <functional>

struct Request
{
    request_method_t method;
    std::string url;
    std::string content;
    std::shared_ptr<void> param;
    std::function<void(request_result_t *result, const Request& r)> completer;
};

class RequestQueue
{
private:
    struct RequestItem
    {
        Request r;
        request_handle_t h;
        RequestQueue* t;
    };
    typedef std::shared_ptr<RequestItem> RequestItemPtr;
public:
    RequestQueue();
    ~RequestQueue();

    const Request* find(std::function<bool(const Request&)> pred);
    bool empty();
    void pushBack(const Request& r);
    void pushFront(const Request& r);
    void clear();
    void start();
    void stop();
private:
    void process();
    static unsigned int completer(request_result_t *result);
private:
    std::list<RequestItemPtr> m_waiting;
    std::list<RequestItemPtr> m_running;
    std::mutex m_mutex;
};