#include "mapper232.h"


namespace nes {

c_mapper232::c_mapper232()
{
    //Quattro *
    mapperName = "Mapper 232";
}

void c_mapper232::reset()
{
    bank = 0;
    page = 0;
    sync();
}

void c_mapper232::sync()
{
    SetPrgBank16k(PRG_8000, bank | page);
    SetPrgBank16k(PRG_C000, bank | 3);
}

void c_mapper232::write_byte(unsigned short address, unsigned char value)
{
    switch (address & 0xC000)
    {
    case 0x8000:
        bank = (value & 0x18) >> 1;
        sync();
        break;
    case 0xC000:
        page = value & 0x3;
        sync();
        break;
    default:
        c_mapper::write_byte(address, value);
        break;
    }
}

} //namespace nes
