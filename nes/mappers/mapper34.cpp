#include "mapper34.h"


namespace nes {

c_mapper34::c_mapper34()
{
    mapperName = "BxROM";
}

c_mapper34::~c_mapper34()
{
}

void c_mapper34::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        SetPrgBank32k(value);
    }
    else
        c_mapper::WriteByte(address, value);
}

} //namespace nes
