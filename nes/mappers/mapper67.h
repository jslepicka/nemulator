#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper67 :
    public c_mapper
{
public:
    c_mapper67();
    ~c_mapper67();
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
private:
    int irq_counter;
    int irq_enabled;
    int ticks;
    int irq_write;
};

} //namespace nes
