#include "mapper146.h"


c_mapper146::c_mapper146()
{
    //Sachen
    //Master Chu & The Drunkard Hu
    mapperName = "Mapper 146";
}

void c_mapper146::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x4100 && address < 0x6000)
    {
        if (address & 0x100)
        {
            SetPrgBank32k(value >> 3);
            SetChrBank8k(value);
        }
    }
    else
        c_mapper::WriteByte(address, value);
}
