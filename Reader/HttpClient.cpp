#include <stdafx.h>
#include "HttpClient.h"
#include <regex>
#include <process.h>
#include "Utils.h"


typedef enum ev_state_t
{
    ev_idle,
    ev_req_proxy,
    ev_rsp_proxy,
    ev_send,
    ev_recv_header,
    ev_recv_body
} ev_state_t;

typedef struct ev_data_t
{
    ev_state_t state;
    request_t *req;
    std::string host;
    std::string path;
    int port;

    // recv data
    int ContentLength;
    int RecvedLength;
    int ChunkedSize;
} ev_data_t;

HttpClient::HttpClient()
{
    unsigned threadID;
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    m_hMutex = CreateMutex(NULL, FALSE, NULL);    
    m_hTread = (HANDLE)_beginthreadex(NULL, 0, &EventThread, this, 0, &threadID);
}


HttpClient::~HttpClient()
{
    if (m_hTread)
    {
        TerminateThread(m_hTread, 0);
        CloseHandle(m_hTread);
    }
    if (m_hMutex)
    {
        CloseHandle(m_hMutex);
    }
    WSACleanup();
}

int HttpClient::SetProxy(proxy_t *proxy)
{
    m_Proxy = proxy;
    return succ;
}

int HttpClient::Request(request_t &req)
{
    request_t *r;
    WaitForSingleObject(m_hMutex, INFINITE);
    r = new request_t(req);
    m_Queue.push(r);
    ReleaseMutex(m_hMutex);
    return succ;
}

int HttpClient::OnRequest(request_t *req)
{
    event_t ev;
    ev_data_t *ed = NULL;
    std::string host;
    std::string path;
    int port;
    SOCKET fd = INVALID_SOCKET;

    // parser url
    if (ParseUrl(req->url, host, path, port))
        goto _fail;

    // create socket
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == fd)
        goto _fail;

    // set recv non-blocking
    unsigned long l;
    if (SOCKET_ERROR == ioctlsocket(fd, FIONBIO, &l))
        goto _fail;

    // create event data
    ed = new ev_data_t;
    ed->state = ev_idle;
    ed->req = req;
    ed->host = host;
    ed->path = path;
    ed->port = port;

    // add event
    ev.fd = fd;
    ev.type = EV_WRITE;
    ev.param1 = (unsigned int)ed;
    ev.param2 = (unsigned int)this;
    ev.handler = EventHandler;
    //m_Event.Add(&ev);

    if (ConnectServer(&ev))
        goto _fail;

    return succ;

_fail:
    if (fd != INVALID_SOCKET)
        closesocket(fd);
    if (ed)
        delete ed;
    return fail;
}

int HttpClient::ParseUrl(const std::string &url, std::string &host, std::string &path, int &port)
{
    std::regex e("(https?):\\/\\/([^\\/:]+)(:(\\d+))?(\\/\\S*)?"); //(https?):\/\/([^\/:]+)(:(\d+))?(\/\S*)?
    std::smatch sm;

    std::regex_match(url, sm, e);
    if (sm.size() == 6)
    {
        host = sm[2];
        path = sm[5];
        if (path == "")
        {
            path = "/";
        }
        if (!sm[4].str().empty())
        {
            port = atoi(sm[4].str().c_str());
        }
        else
        {
            port = 80;
        }
        return succ;
    }
    return fail;
}

int HttpClient::GetAddr(const std::string &host, in_addr *addr)
{
    struct hostent *host_ = NULL;

    host_ = gethostbyname(host.c_str());
    if (!host_)
        return fail;
    *addr = *(struct in_addr*)host_->h_addr_list[0];
    return succ;
}

int HttpClient::ConnectServer(event_t *ev)
{
    struct sockaddr_in sin = { 0 };
    in_addr addr;
    ev_data_t *ed = (ev_data_t *)ev->param1;

    sin.sin_family = AF_INET;
    if (m_Proxy->enable)
    {
        sin.sin_addr.s_addr = inet_addr(m_Proxy->addr);
        sin.sin_port = htons(m_Proxy->port);
    }
    else
    {
        if (GetAddr(ed->host, &addr))
        {
            return fail;
        }
        sin.sin_addr = *(struct in_addr *)&addr;
        sin.sin_port = htons(ed->port);
    }

    if (SOCKET_ERROR == connect(ev->fd, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)))
    {
        if (WSAEWOULDBLOCK != WSAGetLastError())
        {
            return fail;
        }
    }

    // add event
    if (m_Proxy->enable)
    {
        ev->type = EV_WRITE;
        ed->state = ev_req_proxy;
        m_Event.Add(ev);
    }
    else
    {
        ev->type = EV_WRITE;
        ed->state = ev_send;
        m_Event.Add(ev);
    }

    return succ;
}

int HttpClient::ConnectProxyReq(event_t *ev)
{
    std::string header;
    std::string auth;
    char basic[256] = { 0 };
    int len;
    int ret;
    ev_data_t *ed = (ev_data_t *)ev->param1;

    if (!m_Proxy->enable)
    {
        return fail;
    }

    auth.append(m_Proxy->user);
    auth.append(":");
    auth.append(m_Proxy->pass);
    Utils::b64_encode(auth.c_str(), auth.size(), basic, &len);

    // proxy user and password: Ynl1Om9qNGYwcGRsMTc is "byu:oj4f0pdl17" with base64 encode
    // CONNECT www.baidu.com:80 HTTP/1.1\r\nProxy-Authorization: Basic Ynl1Om9qNGYwcGRsMTc\r\n\r\n
    header.append("CONNECT ");
    header.append(ed->host);
    header.append(":");
    header.append(std::to_string((_Longlong)ed->port));
    header.append(" HTTP/1.1\r\nProxy-Authorization: Basic ");
    header.append(basic);
    header.append("\r\n\r\n");

    int written = 0;
    while (1)
    {   
        ret = send(ev->fd, header.c_str() + written, header.size() - written, 0);
        if (ret > 0)
        {
            written += ret;
            if (written == header.size())
            {
                // send completed
                break;
            }
        }
        else if (ret == 0)
        {
            // socket closed
            return fail;
        }
        else if (SOCKET_ERROR == ret)
        {
            if (ret == SOCKET_ERROR && WSAEWOULDBLOCK == WSAGetLastError())
            {
                continue;
            }
            return fail;
        }
    }

    // add event
    ev->type = EV_READ;
    ed->state = ev_rsp_proxy;
    m_Event.Add(ev);
    return succ;
}

int HttpClient::ConnectProxyRsp(event_t *ev)
{
    ev_data_t *ed = (ev_data_t *)ev->param1;
    char buf[256] = { 0 };
    int offset = 0;
    int ret;

    // if success proxy server will response "HTTP/1.0 200 Connection established\r\n\r\n"
    while (1)
    {
        ret = recv(ev->fd, buf + offset, 1, 0);
        if (ret > 0)
        {
            offset++;
            if (offset >= 4)
            {
                if ('\n' == buf[offset - 1]
                    && '\r' == buf[offset - 2]
                    && '\n' == buf[offset - 3]
                    && '\r' == buf[offset - 4])
                {
                    break;
                }
            }
        }
        else if (ret == 0)
        {
            // socket closed
            return fail;
        }
        else
        {
            if (ret == SOCKET_ERROR && WSAEWOULDBLOCK == WSAGetLastError())
            {
                // non-blocking
                continue;
            }
            return fail;
        }
    }

    // TODO... check response
    if (!strstr(buf, "200 Connection established"))
    {
        return fail;
    }

    // add event
    ev->type = EV_WRITE;
    ed->state = ev_send;
    m_Event.Add(ev);
    return succ;
}

int HttpClient::SendRequest(event_t *ev)
{
    ev_data_t *ed = (ev_data_t *)ev->param1;
    std::string header;
    std::string body;
    int ret;

    if (ed->req->method == GET)
        header.append("GET ");
    else
        header.append("POST ");
    header.append(ed->path);
    header.append(" HTTP/1.1\r\n");
    header.append("Host: ");
    header.append(ed->host);
    header.append("\r\n");
    if (ed->req->method == POST)
    {
        header.append("Content-Type: application/x-www-form-urlencoded\r\n");
        header.append("Content-Length: ");
        header.append(std::to_string((_Longlong)ed->req->json.size()));
        header.append("\r\n");
    }
    header.append("User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.99 Safari/537.36\r\n");
    if (ed->req->cookie.size() > 0)
    {
        header.append("Cookie: ");
        header.append(ed->req->cookie);
        header.append("\r\n");
    }
    header.append("\r\n");

    // add body
    if (ed->req->method == POST)
    {
        header.append(ed->req->json);
    }

    int written = 0;
    while (1)
    {
        ret = send(ev->fd, header.c_str() + written, header.size() - written, 0);
        if (ret > 0)
        {
            written += ret;
            if (written == header.size())
            {
                // send completed
                break;
            }
        }
        else if (ret == 0)
        {
            // socket closed
            return fail;
        }
        else if (SOCKET_ERROR == ret)
        {
            if (ret == SOCKET_ERROR && WSAEWOULDBLOCK == WSAGetLastError())
            {
                continue;
            }
            return fail;
        }
    }

    // add event
    ev->type = EV_READ;
    ed->state = ev_recv_header;
    m_Event.Add(ev);

    return succ;
}

int HttpClient::RecvHeader(event_t *ev)
{
    ev_data_t *ed = (ev_data_t *)ev->param1;
    char *header = (char *)malloc(4096);
    int offset = 0;
    int ret;
    int i;
    char *finder, *finder2;

    while (1)
    {
        ret = recv(ev->fd, header + offset, 1, 0);
        if (ret > 0)
        {
            offset++;
            if (offset >= 4096)
            {
                header = (char *)realloc(header, offset + 4096);
            }
            if (offset >= 4)
            {
                if ('\n' == header[offset - 1]
                    && '\r' == header[offset - 2]
                    && '\n' == header[offset - 3]
                    && '\r' == header[offset - 4])
                {
                    // recv header finished.
                    header[offset] = 0;
                    break;
                }
            }
        }
        else if (ret == 0)
        {
            // socket closed
            return fail;
        }
        else
        {
            if (ret == SOCKET_ERROR && WSAEWOULDBLOCK == WSAGetLastError())
            {
                // non-blocking
                continue;
            }
            // recv error
            free(header);
            return fail;
        }
    }

    // parse 'Content-Length'
    ed->ContentLength = 0;
    ed->RecvedLength = 0;
    ed->ChunkedSize = -1;
    for (i = 0; i < offset; i++)
    {
        header[i] = tolower(header[i]);
    }
    finder = strstr(header, "content-length");
    if (finder)
    {
        finder = strstr(finder, ":");
        finder++;
        finder2 = strstr(finder, "\r\n");
        *finder2 = 0;
        ed->ContentLength = atoi(finder);
    }
    else
    {
        // Transfer-Encoding: chunked
        ed->ContentLength = -1;
    }

    free(header);

    // add event
    ev->type = EV_READ;
    ed->state = ev_recv_body;
    m_Event.Add(ev);

    return succ;
}

int HttpClient::RecvBody(event_t *ev)
{
    const int BUF_SIZE = 4096;
    ev_data_t *ed = (ev_data_t *)ev->param1;
    char buf[BUF_SIZE] = { 0 };
    int ret;
    int len;

    if (ed->ContentLength == -1)
    {
        // Transfer-Encoding: chunked
        if (ed->ChunkedSize == -1)
        {
            ret = ReadChunkedSize(ev->fd, ed->ChunkedSize);
            if (ret == succ)
            {
                if (ed->ChunkedSize == 0)
                {
                    // recv finished
                    goto _completed;
                }
                else if (ed->ChunkedSize < 0)
                    return fail;
            }
            else if (ret == clos)
            {
                return fail;
            }
            else if (ret == nonb)
            {
                ed->ChunkedSize = -1;
                goto _continue;
            }
            else
            {
                return fail;
            }
        }

        if (ed->ChunkedSize > 0)
        {
            len = ed->ChunkedSize > BUF_SIZE ? BUF_SIZE : ed->ChunkedSize;
            ret = recv(ev->fd, buf, len, 0);
            if (ret > 0)
            {
                ed->ChunkedSize -= ret;
                if (ed->ChunkedSize == 0)
                {
                    ed->ChunkedSize = -1; // next event call ReadChunkedSize
                }
                if (ed->req->writer)
                {
                    ed->req->writer(buf, 1, ret, ed->req->stream);
                }
                goto _continue;
            }
            else if (ret == 0)
            {
                // socket closeds
                goto _completed;
            }
            else
            {
                if (ret == SOCKET_ERROR && WSAEWOULDBLOCK == WSAGetLastError())
                {
                    // non-blocking
                    goto _continue;
                }
                return fail;
            }
        }
    }
    else
    {
        if (ed->ContentLength > ed->RecvedLength)
        {
            len = (ed->ContentLength - ed->RecvedLength) > BUF_SIZE ? BUF_SIZE : (ed->ContentLength - ed->RecvedLength);

            ret = recv(ev->fd, buf, len, 0);
            if (ret > 0)
            {
                ed->RecvedLength += ret;
                if (ed->req->writer)
                {
                    ed->req->writer(buf, 1, ret, ed->req->stream);
                }
                if (ed->req->progresser)
                {
                    ed->req->progresser(ed->req->param, ed->ContentLength, ed->RecvedLength, 0, 0);
                }
                if (ed->ContentLength == ed->RecvedLength)
                    goto _completed;
                goto _continue;
            }
            else if (ret == 0)
            {
                // socket closeds
                goto _completed;
            }
            else
            {
                if (ret == SOCKET_ERROR && WSAEWOULDBLOCK == WSAGetLastError())
                {
                    // non-blocking
                    goto _continue;
                }
                return fail;
            }
        }
        else
        {
            goto _completed;
        }
    }

_continue:
    // add event
    ev->type = EV_READ;
    ed->state = ev_recv_body;
    m_Event.Add(ev);

    return succ;

_completed:
    return comp;
}

int HttpClient::ReadChunkedSize(SOCKET fd, int &size)
{
    char buf[16] = { 0 };
    int offset = 0;
    int ret;

    while (1)
    {
        ret = recv(fd, buf + offset, 1, 0);
        if (ret > 0)
        {
            offset++;
            if (offset >= 2)
            {
                if ('\n' == buf[offset - 1]
                    && '\r' == buf[offset - 2])
                {
                    if (offset == 2)
                    {
                        offset = 0;
                        continue;
                    }

                    buf[offset - 2] = 0;
                    sscanf(buf, "%x\r\n", &size);
                    return succ;
                }
            }
        }
        else if (ret == 0)
        {
            // socket closed
            return clos;
        }
        else
        {
            if (ret == SOCKET_ERROR && WSAEWOULDBLOCK == WSAGetLastError())
            {
                // non-blocking
                return nonb;
            }
            return fail;
        }
    }
    return succ;
}

unsigned __stdcall HttpClient::EventThread(void* arg)
{
    HttpClient *_this = (HttpClient *)arg;
    request_t *req = NULL;

    while (1)
    {
        WaitForSingleObject(_this->m_hMutex, INFINITE);
        if (!_this->m_Queue.empty())
        {
            req = _this->m_Queue.front();
            _this->m_Queue.pop();
            ReleaseMutex(_this->m_hMutex);
            if (_this->OnRequest(req))
            {
                if (req->completer)
                {
                    req->completer(0, req);
                }
                delete req;
            }
        }
        else
        {
            ReleaseMutex(_this->m_hMutex);
        }

        _this->m_Event.Dispatch();
    }

    _endthreadex(0);
    return succ;
}

void HttpClient::EventHandler(event_t *ev)
{
    int ret;
    ev_data_t *_ed = (ev_data_t *)ev->param1;
    HttpClient *_this = (HttpClient *)ev->param2;

    switch (_ed->state)
    {
    case ev_req_proxy:
        ret = _this->ConnectProxyReq(ev);
        break;
    case ev_rsp_proxy:
        ret = _this->ConnectProxyRsp(ev);
        break;
    case ev_send:
        ret = _this->SendRequest(ev);
        break;
    case ev_recv_header:
        ret = _this->RecvHeader(ev);
        break;
    case ev_recv_body:
        ret = _this->RecvBody(ev);
        if (ret == comp)
        {
            // request completed
            closesocket(ev->fd);
            if (_ed->req->completer)
            {
                _ed->req->completer(1, _ed->req);
            }
            delete _ed->req;
            delete _ed;
        }
        break;
    default:
        break;
    }

    if (ret == fail)
    {
        // error
        closesocket(ev->fd);
        if (_ed->req->completer)
        {
            _ed->req->completer(0, _ed->req);
        }
        delete _ed->req;
        delete _ed;
    }
}