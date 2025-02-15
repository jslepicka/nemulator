#include "mapper93.h"


namespace nes {

c_mapper93::c_mapper93()
{
    //Fantasy Zone
    mapperName = "Mapper 93";
}

c_mapper93::~c_mapper93()
{
}

void c_mapper93::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        if (value & 0x01)
            set_mirroring(MIRRORING_HORIZONTAL);
        else
            set_mirroring(MIRRORING_VERTICAL);
        SetPrgBank16k(PRG_8000, (value & 0xF0) >> 4);
    }
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper93::reset()
{
    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}

} //namespace nes
