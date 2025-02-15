#include "mapper184.h"


namespace nes {

c_mapper184::c_mapper184()
{
    //Atlantis no Nazo
    mapperName = "Mapper 184";
}

c_mapper184::~c_mapper184()
{
}

void c_mapper184::WriteByte(unsigned short address, unsigned char value)
{
    switch (address >> 12)
    {
    case 6:
    case 7:
        SetChrBank4k(CHR_0000, value & 0x07);
        SetChrBank4k(CHR_1000, (value >> 4) & 0x07);
        break;
    default:
        c_mapper::WriteByte(address, value);
        break;
    }
}

} //namespace nes
