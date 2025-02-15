#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper18 :
    public c_mapper
{
public:
    c_mapper18();
    ~c_mapper18() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
private:
    int ticks;
    int irq_enabled;
    int irq_counter;
    int irq_reload;
    int irq_size;
    int irq_asserted;
    int irq_mask;
    void sync();
    int chr[8];
    int prg[3];
    void ack_irq();
};

} //namespace nes
