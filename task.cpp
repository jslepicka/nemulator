#include "task.h"

std::list<c_task *> *c_task::task_list = NULL;

c_task::c_task()
{
	if (task_list == NULL)
		task_list = new std::list<c_task*>;
	dead = false;
}

c_task::~c_task()
{
	//if (task_list)
	//	delete task_list;
}

void c_task::add_task(c_task *task, void *params)
{
	task_list->push_front(task);
	task->init(params);
}