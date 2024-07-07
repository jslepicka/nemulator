#pragma once
#include "..\mapper.h"

class c_mapper16 :
    public c_mapper
{
public:
    c_mapper16(int submapper = 0);
    ~c_mapper16();
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
private:
    unsigned char *eprom;
    int irq_counter;
    int irq_enabled;
    int irq_asserted;
    int ticks;
};