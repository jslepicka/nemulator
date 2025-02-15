#include "mapper105.h"
#include "..\cpu.h"

namespace nes {

c_mapper105::c_mapper105()
{
    mapperName = "NWC";
}

void c_mapper105::sync_prg()
{
    switch (init)
    {
    case 0:
        if (regs[1] & 0x10)
            init++;
        break;
    case 1:
        if (!(regs[1] & 0x10))
            init++;
        break;
    default:
        break;
    }

    if (init >= 2)
    {
        if (regs[1] & 0x08)
        {
            if (regs[0] & 0x08)
            {
                if (regs[0] & 0x4)
                {
                    SetPrgBank16k(PRG_8000, (regs[3] & 0x7) | 0x8);
                    SetPrgBank16k(PRG_C000, 0xF);
                }
                else
                {
                    SetPrgBank16k(PRG_8000, 0x8);
                    SetPrgBank16k(PRG_C000, (regs[3] & 0x7) & 0x8);
                }
            }
            else
            {
                SetPrgBank32k(((regs[3] >> 1) & 0x3) | 0x4);
            }
        }
        else
        {
            SetPrgBank32k((regs[1] >> 1) & 0x3);
        }
    }
    else
    {
        SetPrgBank32k(0);
    }
}

void c_mapper105::sync_chr()
{
}

void c_mapper105::reset()
{
    irq_counter = 0;
    irq_asserted = 0;
    init = 0;
    ticks = 0;
}

void c_mapper105::clock(int cycles)
{
    ticks += cycles;
    while (ticks > 2)
    {
        if (regs[1] & 0x10)
        {
            irq_counter = 0;
            if (irq_asserted)
            {
                irq_asserted = 0;
                cpu->clear_irq();
            }
        }
        else
        {
            irq_counter++;
            if (irq_counter == irq_trigger && !irq_asserted)
            {
                cpu->execute_irq();
                irq_asserted = 1;
            }
        }
        ticks -= 3;
    }
    c_mapper1::clock(cycles);
}

int c_mapper105::get_nwc_time()
{
    int remaining_ticks = irq_trigger - (irq_counter > irq_trigger ? irq_trigger : irq_counter);
    return remaining_ticks / 1786840;
}

} //namespace nes
