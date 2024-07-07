#include "tasklist.h"

c_tasklist::c_tasklist()
{
    stack = new std::stack<c_task*>;
}

c_tasklist::~c_tasklist()
{
    if (stack)
        delete stack;
}

void c_tasklist::push(c_task *task)
{
    stack->push(task);
}

void c_tasklist::pop()
{
    stack->pop();
}