#include "mapper41.h"


namespace nes {

c_mapper41::c_mapper41()
{
    mapperName = "Caltron 6-in-1";
}

void c_mapper41::reset()
{
    SetPrgBank32k(0);
}

void c_mapper41::write_byte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        if (enable_reg2)
        {
            chr = (chr & 0xC) | (value & 0x3);
            SetChrBank8k(chr);
        }
    }
    else if (address >= 0x6000 && address < 0x6800)
    {
        enable_reg2 = address & 0x4;
        SetPrgBank32k(address & 0x7);
        if (address & 0x20)
            set_mirroring(MIRRORING_HORIZONTAL);
        else
            set_mirroring(MIRRORING_VERTICAL);
        chr = (chr & 0x3) | ((address >> 1) & 0xC);
        SetChrBank8k(chr);
    }
    else
        c_mapper::write_byte(address, value);
}

} //namespace nes
