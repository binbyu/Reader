#include "RequestQueue.h"
#include "conv.h"

RequestQueue::RequestQueue()
{
}

RequestQueue::~RequestQueue()
{
    stop();
}

const Request* RequestQueue::find(std::function<bool(const Request&)> pred)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& item : m_running)
    {
        if (pred(item->r)) return &item->r;
    }
    for (auto& item : m_waiting)
    {
        if (pred(item->r)) return &item->r;
    }
    return nullptr;
}

bool RequestQueue::empty()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_waiting.empty() && m_running.empty();
}

void RequestQueue::pushBack(const Request& r)
{
    RequestItemPtr item = std::make_shared<RequestItem>();
    item->r = r;
    item->h = nullptr;
    item->t = this;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_waiting.push_back(item);
    }
}

void RequestQueue::pushFront(const Request& r)
{
    RequestItemPtr item = std::make_shared<RequestItem>();
    item->r = r;
    item->h = nullptr;
    item->t = this;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_waiting.push_front(item);
    }
}

void RequestQueue::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_waiting.clear();
    for (auto& item : m_running)
    {
        if (item->h)
        {
            https_cancel(item->h);
            item->h = nullptr;
        }
    }
    m_running.clear();
}

void RequestQueue::start()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    process();
}

void RequestQueue::process()
{
    while (m_running.size() < MAX_REQUEST_COUNT && !m_waiting.empty())
    {
        auto item = m_waiting.front();
        request_t r;
        memset(&r, 0, sizeof(r));
        r.method = item->r.method;
        r.url = (char*)item->r.url.c_str();
        r.content = item->r.content.empty() ? nullptr : (char *)item->r.content.c_str();
        r.content_length = (int)item->r.content.size();
        r.completer = completer;
        r.param1 = item.get();
        item->h = https_request(&r);
        m_running.push_back(item);
        m_waiting.pop_front();
    }
}

void RequestQueue::stop()
{
    clear();
}

unsigned int RequestQueue::completer(request_result_t *result)
{
    if (result->errno_ == ECAN) return 0;

    RequestItem* item = (RequestItem*)result->req->param1;
    RequestQueue* this_ = item->t;

    if (item->r.completer)
    {
        item->r.completer(result, item->r);
    }
    {
        std::lock_guard<std::mutex> lock(item->t->m_mutex);
        this_->m_running.remove_if([item](const std::shared_ptr<RequestItem>& i) { return i.get() == item; });
        this_->process();
    }
    return 0;
}