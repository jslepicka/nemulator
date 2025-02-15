#include "mapper40.h"

#include "..\cpu.h"

namespace nes {

c_mapper40::c_mapper40()
{
    mapperName = "Mapper 40";
}

c_mapper40::~c_mapper40()
{
}

void c_mapper40::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        switch (address >> 12)
        {
        case 8:
        case 9:
            irq_enabled = 0;
            irq_counter = 0;
            cpu->clear_irq();
            break;
        case 0xA:
        case 0xB:
            irq_enabled = 1;
            break;
        case 0xC:
        case 0xD:
            break;
        case 0xE:
        case 0xF:
            SetPrgBank8k(PRG_C000, value & 0x7);
            break;
        default:
            break;
        }
    }
    else
        c_mapper::WriteByte(address, value);
}

unsigned char c_mapper40::ReadByte(unsigned short address)
{
    if (address >= 0x6000 && address < 0x8000)
    {
        return *((pPrgRom + 0x2000 * 6) + (address & 0x1FFF));
    }
    return *(prgBank[(address >> 13) & 3] + (address & 0x1FFF));
}

void c_mapper40::reset()
{
    irq_counter = 0;
    irq_enabled = 0;
    ticks = 0;
    SetPrgBank8k(PRG_8000, 4);
    SetPrgBank8k(PRG_A000, 5);
    SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
}

void c_mapper40::clock(int cycles)
{
    if (irq_enabled)
    {
        ticks += cycles;
        while (ticks > 2)
        {
            if (++irq_counter == 4096)
            {
                cpu->execute_irq();
                irq_counter = 0;
            }
            ticks -= 3;
        }
    }
}

} //namespace nes
