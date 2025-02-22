#include "mapper8.h"

namespace nes {

c_mapper8::c_mapper8()
{
    mapperName = "FFE F3xxx";
}

c_mapper8::~c_mapper8()
{
}

void c_mapper8::write_byte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        SetChrBank8k(value & 0x7);
        SetPrgBank16k(PRG_8000, value >> 3);
    }
    else
        c_mapper::write_byte(address, value);
}

void c_mapper8::reset()
{
    SetPrgBank16k(PRG_C000, 1);
}

} //namespace nes
