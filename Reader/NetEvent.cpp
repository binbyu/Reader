#include "stdafx.h"
#include "NetEvent.h"


NetEvent::NetEvent()
{
    FD_ZERO(&m_readfds);
    FD_ZERO(&m_writefds);
    FD_ZERO(&m_exceptfds);
}

NetEvent::~NetEvent()
{
    event_map_t::iterator it;
    for (it = m_readmap.begin(); it != m_readmap.end(); it++)
    {
        closesocket(it->second->fd);
        delete it->second;
    }
    for (it = m_writemap.begin(); it != m_writemap.end(); it++)
    {
        closesocket(it->second->fd);
        delete it->second;
    }
    for (it = m_exceptmap.begin(); it != m_exceptmap.end(); it++)
    {
        closesocket(it->second->fd);
        delete it->second;
    }
    m_readmap.clear();
    m_writemap.clear();
    m_exceptmap.clear();
}

int NetEvent::Add(event_t *ev)
{
    fd_set          *s = NULL;
    event_map_t     *m = NULL;
    event_t         *e = NULL;
    if (ev->type & EV_READ)
    {
        m = &m_readmap;
        s = &m_readfds;
    }
    else if (ev->type & EV_WRITE)
    {
        m = &m_writemap;
        s = &m_writefds;
    }
    else if (ev->type & EV_EXCEPT)
    {
        m = &m_exceptmap;
        s = &m_exceptfds;
    }
    if (!m || !s)
    {
        return 1;
    }

    if (s->fd_count >= FD_SETSIZE)
    {
        return 2; // full
    }

    if (m->end() != m->find(ev->fd))
    {
        return 3; // already exist
    }

    e = new event_t(*ev);
    m->insert(std::make_pair(ev->fd, e));
    FD_SET(ev->fd, s);
    return 0;
}

int NetEvent::Del(event_t *ev)
{
    fd_set          *s = NULL;
    event_map_t     *m = NULL;
    event_map_t::iterator it;
    if (ev->type & EV_READ)
    {
        m = &m_readmap;
        s = &m_readfds;
    }
    else if (ev->type & EV_WRITE)
    {
        m = &m_writemap;
        s = &m_writefds;
    }
    else if (ev->type & EV_EXCEPT)
    {
        m = &m_exceptmap;
        s = &m_exceptfds;
    }
    if (!m || !s)
    {
        return 1;
    }

    it = m->find(ev->fd);
    if (m->end() == it)
    {
        return 4; // not found
    }

    delete it->second;
    m->erase(it);
    FD_CLR(ev->fd, s);
    return 0;
}

int NetEvent::Del(unsigned int fd)
{
    event_map_t::iterator it;

    it = m_readmap.find(fd);
    if (m_readmap.end() != it)
    {
        delete it->second;
        m_readmap.erase(it);
        FD_CLR(fd, &m_readfds);
    }

    it = m_writemap.find(fd);
    if (m_writemap.end() != it)
    {
        delete it->second;
        m_writemap.erase(it);
        FD_CLR(fd, &m_writefds);
    }

    it = m_exceptmap.find(fd);
    if (m_exceptmap.end() != it)
    {
        delete it->second;
        m_exceptmap.erase(it);
        FD_CLR(fd, &m_exceptfds);
    }
    return 0;
}

int NetEvent::Dispatch()
{
    fd_set readfds = { 0 };
    fd_set writefds = { 0 };
    fd_set exceptfds = { 0 };
    event_t *active_evs[FD_SETSIZE * 3];
    event_map_t::iterator it;
    unsigned int active_size = 0;
    struct timeval timeout = { 0, 500000 };
    int ret;
    unsigned int i;

    //while (1)
    {
        if (0 == m_readfds.fd_count + m_writefds.fd_count + m_exceptfds.fd_count)
        {
            Sleep(500);
            //continue;
            return 0;
        }

        active_size = 0;
        memcpy(&readfds, &m_readfds, sizeof(m_readfds.fd_count) + m_readfds.fd_count * sizeof(SOCKET));
        memcpy(&writefds, &m_writefds, sizeof(m_writefds.fd_count) + m_writefds.fd_count * sizeof(SOCKET));
        memcpy(&exceptfds, &m_exceptfds, sizeof(m_exceptfds.fd_count) + m_exceptfds.fd_count * sizeof(SOCKET));

        ret = select(0, &readfds, &writefds, &exceptfds, &timeout);
        switch (ret)
        {
        case 0: // the time limit expired
            break;
        case SOCKET_ERROR: // an error occurred
            return 1;
        default: // the total number of socket handles that are ready
            for (i = 0; i < readfds.fd_count; i++)
            {
                it = m_readmap.find(readfds.fd_array[i]);
                if (m_readmap.end() != it && it->second)
                {
                    active_evs[active_size++] = it->second;
                    if (!(it->second->type & EV_PERSIST))
                    {
                        FD_CLR(it->first, &m_readfds);
                        m_readmap.erase(it);
                    }
                }
            }
            for (i = 0; i < writefds.fd_count; i++)
            {
                it = m_writemap.find(writefds.fd_array[i]);
                if (m_writemap.end() != it && it->second)
                {
                    active_evs[active_size++] = it->second;
                    if (!(it->second->type & EV_PERSIST))
                    {
                        FD_CLR(it->first, &m_writefds);
                        m_writemap.erase(it);
                    }
                }
            }
            for (i = 0; i < exceptfds.fd_count; i++)
            {
                it = m_exceptmap.find(exceptfds.fd_array[i]);
                if (m_exceptmap.end() != it && it->second)
                {
                    active_evs[active_size++] = it->second;
                    if (!(it->second->type & EV_PERSIST))
                    {
                        FD_CLR(it->first, &m_exceptfds);
                        m_exceptmap.erase(it);
                    }
                }
            }
            break;
        }

        for (i = 0; i < active_size; i++)
        {
            active_evs[i]->handler(active_evs[i]);
            if (!(active_evs[i]->type & EV_PERSIST))
                delete active_evs[i];
        }
    }
    return 0;
}