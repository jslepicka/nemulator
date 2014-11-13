#pragma once
#include <stack>
#include "task.h"

class c_tasklist
{
public:
	c_tasklist(void);
	~c_tasklist(void);
	void push(c_task *task);
	void pop();
protected:
	std::stack<c_task*> *stack;
};
