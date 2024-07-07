#pragma once
#include "mapper1.h"

class c_mapper105 :
    public c_mapper1
{
public:
    c_mapper105();
    ~c_mapper105() {};
    void reset();
    void clock(int cycles);
    int get_nwc_time();
private:
    int init;
    int irq_counter;
    int irq_asserted;
    void sync_prg();
    void sync_chr();
    int ticks;
    static const int irq_trigger = 0x28000000;
};