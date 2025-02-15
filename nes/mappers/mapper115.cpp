#include "mapper115.h"


namespace nes {

c_mapper115::c_mapper115()
{
    //AV Jiu Ji Ma Jiang 2 (Unl)
    //Yuu Yuu Hakusho Final - Makai Saikyou Retsuden (English) (Unl)
    mapperName = "Mapper 115";
}

void c_mapper115::WriteByte(unsigned short address, unsigned char value)
{

    switch (address >> 12)
    {
    case 0x6:
    case 0x7:
        if (address & 0x1)
        {
            chr_offset = (value & 0x1) << 8;
        }
        else
        {
            reg1 = value;
        }
        Sync();
        break;
    default:
        c_mapper4::WriteByte(address, value);
        break;
    }
}

void c_mapper115::reset()
{
    reg1 = 0;
    c_mapper4::reset();
}

void c_mapper115::Sync()
{
    c_mapper4::Sync();
    if (reg1 & 0x80)
    {
        SetPrgBank16k(PRG_8000, reg1 & 0xF);
    }
}

} //namespace nes
