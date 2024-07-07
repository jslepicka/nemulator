#include "mapper_mc_acc.h"

c_mapper_mc_acc::c_mapper_mc_acc()
{
    //Incredible Crash Dummies
    mapperName = "MC-ACC";
}

c_mapper_mc_acc::~c_mapper_mc_acc()
{
}

void c_mapper_mc_acc::fire_irq()
{
    irq_delay = 4;
}

void c_mapper_mc_acc::clock(int cycles)
{
    if (irq_delay > 0)
    {
        irq_delay -= cycles;
        if (irq_delay <= 0)
        {
            c_mapper4::fire_irq();
        }
    }
    if (!(current_address & 0x1000))
    {
        low_count -= cycles;
        if (low_count < 0)
            low_count = 0;
    }
    else
        low_count = 35;
}

void c_mapper_mc_acc::reset()
{
    irq_delay = 0;
    c_mapper4::reset();
}