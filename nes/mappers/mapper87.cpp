#include "mapper87.h"


c_mapper87::c_mapper87()
{
    //City Connection (J)
    mapperName = "Mapper 87";
}


void c_mapper87::WriteByte(unsigned short address, unsigned char value)
{
    switch (address >> 12)
    {
    case 6:
    case 7:
        SetChrBank8k(((value & 0x1) << 1) | (value & 0x2) >> 1);
        break;
    default:
        c_mapper::WriteByte(address, value);
    }
}