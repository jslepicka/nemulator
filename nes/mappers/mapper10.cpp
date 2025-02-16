#include "mapper10.h"


namespace nes {

c_mapper10::c_mapper10()
{
    //Fire Emblem
    mapperName = "MMC4";
}

void c_mapper10::reset()
{
    c_mapper9::reset();
    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}

void c_mapper10::write_byte(unsigned short address, unsigned char value)
{
    if (address >> 12 == 0xA)
    {
        SetPrgBank16k(PRG_8000, value & 0xF);
    }
    else
        c_mapper9::write_byte(address, value);
}

} //namespace nes
