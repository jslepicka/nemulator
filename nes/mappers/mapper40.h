#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper40 :
    public c_mapper
{
public:
    c_mapper40();
    ~c_mapper40();
    void WriteByte(unsigned short address, unsigned char value);
    unsigned char ReadByte(unsigned short address);
    void reset();
    void clock(int cycles);
private:
    int irq_counter;
    int irq_enabled;
    int ticks;
};

} //namespace nes
