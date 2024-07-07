#pragma once
#include "mapper4.h"

class c_mapper_mc_acc :
    public c_mapper4
{
public:
    c_mapper_mc_acc();
    ~c_mapper_mc_acc();
    void clock(int cycles);
    void reset();
protected:
    void fire_irq();
private:
    int irq_delay;
};