#include "mapper152.h"


namespace nes {

c_mapper152::c_mapper152()
{
    //Arkanoid II (J)
    mapperName = "Mapper 152";
}


void c_mapper152::write_byte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        if (value & 0x80)
            set_mirroring(MIRRORING_ONESCREEN_LOW);
        else
            set_mirroring(MIRRORING_ONESCREEN_HIGH);
        SetPrgBank16k(PRG_8000, (value >> 4) & 0x07);
        SetChrBank8k(value & 0x0F);
    }
    else
        c_mapper::write_byte(address, value);
}

void c_mapper152::reset()
{
    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}

} //namespace nes
