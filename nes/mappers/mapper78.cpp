#include "mapper78.h"


namespace nes {

c_mapper78::c_mapper78()
{
    mapperName = "Mapper 78";
    mirror_mode = 0;
}

void c_mapper78::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        SetPrgBank16k(PRG_8000, value & 0x7);
        SetChrBank8k(value >> 4);
        switch (mirror_mode)
        {
        case 0:
            if (value & 0x8)
                set_mirroring(MIRRORING_VERTICAL);
            else
                set_mirroring(MIRRORING_HORIZONTAL);
            break;
        case 1:
            if (value & 0x08)
                set_mirroring(MIRRORING_ONESCREEN_HIGH);
            else
                set_mirroring(MIRRORING_ONESCREEN_LOW);
            break;
        }
    }
    else

        c_mapper::WriteByte(address, value);

}

void c_mapper78::reset()
{
    four_screen = false;

    switch (submapper) {
    case 1:
        mirror_mode = 0;
        break;
    case 2:
        mirror_mode = 1;
        break;
    default:
        break;
    }

    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}

} //namespace nes
