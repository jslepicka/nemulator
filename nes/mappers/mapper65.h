#pragma once
#include "..\mapper.h"

class c_mapper65 :
    public c_mapper
{
public:
    c_mapper65();
    ~c_mapper65() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
private:
    int irq_enabled;
    int irq_asserted;
    int irq_counter;
    int irq_reload_high;
    int irq_reload_low;
    int ticks;
};