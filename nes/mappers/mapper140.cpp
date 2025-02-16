#include "mapper140.h"


namespace nes {

c_mapper140::c_mapper140()
{
    //Bio Senshi Dan - Increaser Tono Tatakai
    mapperName = "Mapper 140";
}

void c_mapper140::write_byte(unsigned short address, unsigned char value)
{
    switch (address >> 12)
    {
    case 6:
    case 7:
        SetChrBank8k(value & 0xF);
        SetPrgBank32k((value >> 4) & 0x3);
        break;
    default:
        c_mapper::write_byte(address, value);
    }
}

} //namespace nes
