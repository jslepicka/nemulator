#include "mapper92.h"


c_mapper92::c_mapper92()
{
    //Moero!! Pro Soccer (J)
    //Moero!! Pro Yakuu '88 - Ketteiban (J)
    mapperName = "Mapper 92";
}

void c_mapper92::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        switch ((value >> 6) & 0x3)
        {
        case 0:
            if (latch & 0x40)
            {
                SetChrBank8k(latch & 0xF);
            }
            if (latch & 0x80)
            {
                SetPrgBank16k(PRG_C000, latch & 0xF);
            }
            break;
        case 1:
        case 2:
            latch = value;
            break;
        case 3:
            break;
        }
    }
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper92::reset()
{
    latch = 0;
    SetPrgBank16k(PRG_8000, 0);
}