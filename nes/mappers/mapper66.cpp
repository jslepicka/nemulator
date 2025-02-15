#include "mapper66.h"


namespace nes {

c_mapper66::c_mapper66()
{
    //SMB + Duckhunt
    mapperName = "GxROM";
}

c_mapper66::~c_mapper66()
{
}

void c_mapper66::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        SetPrgBank32k((value & 0x30) >> 4);
        SetChrBank8k(value & 0x03);
    }
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper66::reset()
{
    SetPrgBank32k(0);
    SetChrBank8k(0);
}

} //namespace nes
