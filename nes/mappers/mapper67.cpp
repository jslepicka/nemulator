#include "mapper67.h"

#include "..\cpu.h"

c_mapper67::c_mapper67()
{
    //Fantasy Zone 2
    mapperName = "Mapper 67";
}

c_mapper67::~c_mapper67()
{
}

void c_mapper67::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        switch (address & 0xF800)
        {
        case 0x8800:
            SetChrBank2k(CHR_0000, value);
            break;
        case 0x9800:
            SetChrBank2k(CHR_0800, value);
            break;
        case 0xA800:
            SetChrBank2k(CHR_1000, value);
            break;
        case 0xB800:
            SetChrBank2k(CHR_1800, value);
            break;
        case 0xC800:
            if (irq_write == 0) //hi
            {
                irq_counter = (irq_counter & 0xFF) | (value << 8);
            }
            else
            {
                irq_counter = (irq_counter & 0xFF00) | value;
            }
            irq_write ^= 1;
            break;
        case 0xD800:
            cpu->clear_irq();
            irq_enabled = (value & 0x10);
            irq_write = 0;
            break;
        case 0xE800:
            switch (value & 0x03)
            {
            case 0:
                set_mirroring(MIRRORING_VERTICAL);
                break;
            case 1:
                set_mirroring(MIRRORING_HORIZONTAL);
                break;
            case 2:
                set_mirroring(MIRRORING_ONESCREEN_LOW);
                break;
            case 3:
                set_mirroring(MIRRORING_ONESCREEN_HIGH);
                break;
            }
            break;
        case 0xF800:
            SetPrgBank16k(PRG_8000, value);
            break;
        default:
            break;
        }
    }
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper67::reset()
{
    irq_counter = 0;
    irq_enabled = 0;
    irq_write = 0;
    ticks = 0;

    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}

void c_mapper67::clock(int cycles)
{
    if (irq_enabled)
    {
        ticks += cycles;
        while (ticks > 2)
        {
            int prev = irq_counter;
            irq_counter = (irq_counter - 1) & 0xFFFF;
            if (irq_counter > prev) // wrap around
            {
                cpu->execute_irq();
                irq_enabled = 0;
            }
            ticks -= 3;
        }
    }
}