#include "StdAfx.h"
#include "MiniStack.h"
#include <assert.h>
#include <string.h>


MiniStack::MiniStack(min_stack_t* data)
    : m_data(NULL)
{
    m_data = data;
    if (!m_data)
    {
        assert(FALSE);
    }
}

MiniStack::~MiniStack(void)
{
    m_data = NULL;
}

int MiniStack::top()
{
    if (0 == size())
    {
        assert(FALSE);
    }
    return m_data->data[m_data->top];
}

void MiniStack::pop()
{
    if (empty())
    {
        return;
    }
    m_data->data[m_data->top] = 0;
    m_data->size--;
    m_data->top = prev();
}

void MiniStack::push(int val)
{
    m_data->top = next();
    m_data->data[m_data->top] = val;
    if (!full())
    {
        m_data->size++;
    }
}

int MiniStack::size()
{
    return m_data->size;
}

void MiniStack::clear()
{
    memset(m_data, 0, sizeof(min_stack_t));
}

bool MiniStack::empty()
{
    return m_data->size == 0;
}

bool MiniStack::full()
{
    return m_data->size == MAX_STACK_LENGTH;
}

int MiniStack::prev()
{
    return (m_data->top+MAX_STACK_LENGTH-1) % MAX_STACK_LENGTH;
}

int MiniStack::next()
{
    return (m_data->top+1) % MAX_STACK_LENGTH;
}