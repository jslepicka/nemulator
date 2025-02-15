#include "mapper97.h"


namespace nes {

c_mapper97::c_mapper97()
{
    //Kaiketsu Yanchamaru (J)
    mapperName = "Mapper 97";
}

void c_mapper97::reset()
{
    SetPrgBank16k(PRG_8000, prgRomPageCount16k-1);
}

void c_mapper97::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        SetPrgBank16k(PRG_C000, value & 0xF);
        switch (value >> 6)
        {
        case 0:
            set_mirroring(MIRRORING_ONESCREEN_LOW);
            break;
        case 1:
            set_mirroring(MIRRORING_HORIZONTAL);
            break;
        case 2:
            set_mirroring(MIRRORING_VERTICAL);
            break;
        case 3:
            set_mirroring(MIRRORING_ONESCREEN_HIGH);
            break;
        }
    }
    else
        c_mapper::WriteByte(address, value);
}

} //namespace nes
