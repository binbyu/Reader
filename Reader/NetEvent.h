#ifndef __NET_EVENT_H__
#define __NET_EVENT_H__


#include <map>

typedef enum
{
    EV_UNKNOWN              = 0x00,
    EV_READ                 = 0x01,
    EV_WRITE                = 0x02,
    EV_EXCEPT               = 0x04,
    EV_PERSIST              = 0x08
} event_type_t;

class event_t
{
public:
    unsigned int            fd;         // socket
    unsigned int            type;
    unsigned int            param1;
    unsigned int            param2;
    void(* handler) (event_t *);

    event_t()
    {
        this->fd = INVALID_SOCKET;
        this->type = EV_UNKNOWN;
        this->param1 = 0;
        this->param1 = 0;
        this->handler = 0;
    }

    event_t(const event_t& ev)
    {
        this->fd = ev.fd;
        this->type = ev.type;
        this->param1 = ev.param1;
        this->param2 = ev.param2;
        this->handler = ev.handler;
    }
};

typedef std::map<unsigned int, event_t *> event_map_t;

class NetEvent
{
public:
    NetEvent();
    ~NetEvent();

    int Add(event_t *ev);
    int Del(event_t *ev);
    int Del(unsigned int fd);
    int Dispatch();

private:
    int Clear();

private:
    fd_set m_readfds;
    fd_set m_writefds;
    fd_set m_exceptfds;

    event_map_t  m_readmap;
    event_map_t  m_writemap;
    event_map_t  m_exceptmap;
};

#endif // !__NET_EVENT_H__
