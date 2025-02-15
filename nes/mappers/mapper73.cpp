#include "mapper73.h"

#include "..\cpu.h"

namespace nes {

c_mapper73::c_mapper73()
{
    mapperName = "VRC3";
}

void c_mapper73::reset()
{
    irq_counter = 0;
    irq_reload = 0;
    irq_mode = 0;
    irq_enabled = 0;
    irq_enable_on_ack = 0;
    irq_asserted = 0;
    ticks = 0;
    SetPrgBank16k(PRG_C000, prgRomPageCount16k-1);
}

void c_mapper73::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        switch (address & 0xF000)
        {
        case 0x8000:
        case 0x9000:
        case 0xA000:
        case 0xB000:
            {
                int x = ((address >> 12) & 0x03) * 4;
                int mask = ~(0xF << x);
                irq_reload = (irq_reload & mask) | ((value & 0xF) << x);
            }
            break;
        case 0xC000:
            irq_mode = value & 0x4;
            irq_enabled = value & 0x2;
            irq_enable_on_ack = value & 0x1;
            if (irq_enabled)
                irq_counter = irq_reload;
            ack_irq();
            break;
        case 0xD000:
            if (irq_enable_on_ack)
                irq_enabled = 0x2;
            else
                irq_enabled = 0;
            ack_irq();
            break;
        case 0xF000:
            SetPrgBank16k(PRG_8000, value & 0xF);
            break;
        }
    }
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper73::ack_irq()
{
    if (irq_asserted)
    {
        cpu->clear_irq();
        irq_asserted = 0;
    }
}

void c_mapper73::clock(int cycles)
{
    if (irq_enabled)
    {
        ticks += cycles;
        while (ticks > 2)
        {
            ticks -= 3;
            if (irq_mode) //8-bit
            {
                int temp = irq_counter & 0xFF;
                temp = ((temp + 1) & 0xFF);
                irq_counter = (irq_counter & 0xFF00) | temp;
                if (temp == 0)
                {
                    if (!irq_asserted)
                    {
                        cpu->execute_irq();
                        irq_asserted = 1;
                    }
                }
            }
            else //16-bit
            {
                irq_counter = ((irq_counter + 1) & 0xFFFF);
                if (irq_counter == 0)
                {
                    if (!irq_asserted)
                    {
                        cpu->execute_irq();
                        irq_asserted = 1;
                    }
                }
            }
        }
    }
}

} //namespace nes
