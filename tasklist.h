#pragma once
#include <stack>
#include "task.h"

class c_tasklist
{
public:
	c_tasklist();
	~c_tasklist();
	void push(c_task *task);
	void pop();
protected:
	std::stack<c_task*> *stack;
};
