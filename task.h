#pragma once
#include <deque>
#include <vector>
#include <list>

import input_handler;

class c_task
{
public:
    c_task();
    virtual ~c_task();
    virtual void resize() {};
    virtual void init(void *params) {};
    virtual int update(double dt, int child_result, void *params) { return 0; };
    virtual void on_pause(bool paused) {};
    virtual void draw() {};
    bool dead;
    static void add_task(c_task *task, void *params);
    enum TASK_RESULTS
    {
        TASK_RESULT_NONE,
        TASK_RESULT_CANCEL,
        TASK_RESULT_RETURN
    };


    static std::list<c_task *> *task_list;

};
