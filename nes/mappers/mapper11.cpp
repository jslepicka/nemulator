#include "mapper11.h"


namespace nes {

c_mapper11::c_mapper11()
{
    mapperName = "Color Dreams";
}

c_mapper11::~c_mapper11()
{
}

void c_mapper11::write_byte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        SetPrgBank32k(value & 0x3);
        SetChrBank8k(value >> 4);
    }
    else
        c_mapper::write_byte(address, value);
}

void c_mapper11::reset()
{
    SetPrgBank32k(0);
    //SetChrBank8k(2);
    //c_mapper::Reset();
}

} //namespace nes
