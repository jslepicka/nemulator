#include "mapper86.h"


namespace nes {

c_mapper86::c_mapper86()
{
    //Moero!! Pro Yakyuu (Red) (J)
    //Moero!! Pro Yakyuu (Black) (J)
    mapperName = "Mapper 86";
}

void c_mapper86::WriteByte(unsigned short address, unsigned char value)
{
    switch (address >> 12)
    {
    case 6:
        SetPrgBank32k((value >> 4) & 0x3);
        SetChrBank8k((value & 0x3) | ((value >> 4) & 0x4));
        break;
    case 7:
        break;
    default:
        c_mapper::WriteByte(address, value);
    }
}

} //namespace nes
