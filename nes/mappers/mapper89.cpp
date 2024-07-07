#include "mapper89.h"


c_mapper89::c_mapper89()
{
    //Tenka no Goikenban - Mito Koumon (J)
    mapperName = "Mapper 89";
}

void c_mapper89::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        if (value & 0x8)
            set_mirroring(MIRRORING_ONESCREEN_HIGH);
        else
            set_mirroring(MIRRORING_ONESCREEN_LOW);

        SetPrgBank16k(PRG_8000, (value >> 4) & 0x7);
        SetChrBank8k((value & 0x7) | ((value >> 4) & 0x8));
    }
}

void c_mapper89::reset()
{
    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}