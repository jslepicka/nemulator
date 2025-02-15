#include "mapper7.h"

namespace nes {

c_mapper7::c_mapper7()
{
    //Battletoads, Wizards and Warriors
    mapperName = "AxROM";
}

c_mapper7::~c_mapper7()
{
}

void c_mapper7::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        SetPrgBank32k(value & 0xF);
        if (value & 0x10)
            set_mirroring(MIRRORING_ONESCREEN_HIGH);
        else
            set_mirroring(MIRRORING_ONESCREEN_LOW);
    }
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper7::reset()
{
    set_mirroring(MIRRORING_ONESCREEN_LOW);
    SetPrgBank32k(0);
}

} //namespace nes
