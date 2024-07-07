#pragma once
#include "..\mapper.h"

class c_mapper42 :
    public c_mapper
{
public:
    c_mapper42();
    ~c_mapper42() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
    unsigned char ReadByte(unsigned short address);
private:
    int irq_enabled;
    int irq_asserted;
    int irq_counter;
    int ticks;
    unsigned char *prg_6k;
};