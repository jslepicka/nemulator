#include "mapper72.h"


namespace nes {

c_mapper72::c_mapper72()
{
    //Pinball Quest (J)
    //Moero!! Juudou Warriors (J)
    //Moero!! Pro Tennis (J)
    mapperName = "Mapper 72";
}

void c_mapper72::write_byte(unsigned short address, unsigned char value)
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
                SetPrgBank16k(PRG_8000, latch & 0xF);
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
        c_mapper::write_byte(address, value);
}

void c_mapper72::reset()
{
    latch = 0;
    //SetPrgBank16k(PRG_8000, 0);
    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}

} //namespace nes
