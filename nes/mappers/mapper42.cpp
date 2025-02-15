#include "mapper42.h"

#include "..\cpu.h"

namespace nes {

c_mapper42::c_mapper42()
{
    //Bio Miracle Bokutte Upa (J) (Mario Baby - FDS Conversion)
    mapperName = "Mapper 42";
}

void c_mapper42::reset()
{
    ticks = 0;
    irq_enabled = 0;
    irq_asserted = 0;
    irq_counter = 0;
    prg_6k = pPrgRom;
    SetPrgBank32k(prgRomPageCount32k - 1);
}

void c_mapper42::clock(int cycles)
{
    if (irq_enabled)
    {
        ticks += cycles;
        while (ticks > 2)
        {
            ticks -= 3;
            if(++irq_counter == 0x6000 && !irq_asserted)
            {
                cpu->execute_irq();
                irq_asserted = 1;
            }
        }
    }
}

unsigned char c_mapper42::ReadByte(unsigned short address)
{
    if (address >= 0x6000 && address < 0x8000)
    {
        return *(prg_6k + (address & 0x1FFF));
    }
    else
        return c_mapper::ReadByte(address);
}

void c_mapper42::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0xE000)
    {
        switch (address & 0x3)
        {
        case 0:
            prg_6k = pPrgRom + ((value % prgRomPageCount8k) * 0x2000);
            break;
        case 1:
            if (value & 0x8)
                set_mirroring(MIRRORING_HORIZONTAL);
            else
                set_mirroring(MIRRORING_VERTICAL);
            break;
        case 2:
            irq_enabled = value & 0x2;
            if (!irq_enabled)
            {
                irq_counter = 0;
                if (irq_asserted)
                {
                    cpu->clear_irq();
                    irq_asserted = 0;
                }
            }
            break;
        case 3:
            break;
        }
    }
}

} //namespace nes
