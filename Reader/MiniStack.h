#ifndef __MINI_STACK_H__
#define __MINI_STACK_H__

#include "types.h"


class MiniStack
{
public:
    MiniStack(min_stack_t* data);
    ~MiniStack(void);

    int top();
    void pop();
    void push(int val);
    int size();
    void clear();

private:
    bool empty();
    bool full();
    int prev();
    int next();

private:
    min_stack_t* m_data;
};

#endif