#include "mapper71.h"


namespace nes {

c_mapper71::c_mapper71()
{
    //Camerica games: Bee 52, Mig 29, etc.
    mapperName = "Mapper 71";
    enable_mirroring_control = 1;
}

void c_mapper71::write_byte(unsigned short address, unsigned char value)
{
    if (address >= 0xC000)
    {
        SetPrgBank16k(PRG_8000, value);
    }
    else if (enable_mirroring_control && address >= 0x8000 && address <= 0x9FFF)
    {
        if (value & 0x10)
            set_mirroring(MIRRORING_ONESCREEN_HIGH);
        else
            set_mirroring(MIRRORING_ONESCREEN_LOW);
    }
    else
        c_mapper::write_byte(address, value);
}

void c_mapper71::reset()
{
    enable_mirroring_control = 0;
    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    if (submapper == 1) {
        enable_mirroring_control = 1;
    }
}

} //namespace nes
