#include "mapper34.h"


namespace nes {

c_mapper34::c_mapper34()
{
    mapperName = "BxROM";
}

c_mapper34::~c_mapper34()
{
}

void c_mapper34::write_byte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        SetPrgBank32k(value);
    }
    else
        c_mapper::write_byte(address, value);
}

} //namespace nes
