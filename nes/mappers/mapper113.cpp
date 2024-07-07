#include "mapper113.h"


c_mapper113::c_mapper113()
{
    //Rad Racket, Papillon
    mapperName = "Mapper 113";
}

void c_mapper113::WriteByte(unsigned short address, unsigned char value)
{
    if ((address & 0x4100) >= 0x4100 && (address & 0x4100) < 0x6000)
    {
        if (value & 0x80)
            set_mirroring(MIRRORING_VERTICAL);
        else
            set_mirroring(MIRRORING_HORIZONTAL);
        SetPrgBank32k((value >> 3) & 0x7);
        SetChrBank8k((value & 0x7) | ((value >> 3) & 0x8));
    }
    else
        c_mapper::WriteByte(address, value);
}