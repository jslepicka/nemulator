#include "mapper2.h"

c_mapper2::c_mapper2()
{
    mapperName = "UxROM";
}

c_mapper2::~c_mapper2()
{
}

void c_mapper2::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
        SetPrgBank16k(PRG_8000, value);
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper2::reset()
{
    c_mapper::reset();
}