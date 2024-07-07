#include "mapper180.h"

c_mapper180::c_mapper180()
{
    //Crazy Climber
    mapperName = "Mapper 180";
}

void c_mapper180::reset()
{
    SetPrgBank16k(PRG_8000, 0);
}

void c_mapper180::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        SetPrgBank16k(PRG_C000, value);
    }
    else
    {
        c_mapper::WriteByte(address, value);
    }
}