#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper19 :
    public c_mapper
{
public:
    c_mapper19();
    ~c_mapper19();
    void WriteByte(unsigned short address, unsigned char value);
    unsigned char ReadByte(unsigned short address);
    void reset();
    void clock(int cycles);
private:
    int ticks;
    void sync_chr();
    void sync_mirroring();
    int irq_counter;
    int irq_enabled;
    unsigned char *chr_ram;
    int chr_regs[8];
    int mirroring_regs[4];
    int reg_e800;
    int irq_asserted;
};

} //namespace nes
