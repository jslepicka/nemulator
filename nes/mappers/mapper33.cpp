#include "mapper33.h"


namespace nes {

c_mapper33::c_mapper33()
{
    //Bubble Bobble 2 (J)
    mapperName = "Mapper 33";
}


void c_mapper33::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        switch (address & 0xA003)
        {
        case 0x8000:
            if (value & 0x40)
                set_mirroring(MIRRORING_HORIZONTAL);
            else
                set_mirroring(MIRRORING_VERTICAL);
            SetPrgBank8k(PRG_8000, value & 0x3F);
            break;
        case 0x8001:
            SetPrgBank8k(PRG_A000, value & 0x3F);
            break;
        case 0x8002:
            SetChrBank2k(CHR_0000, value);
            break;
        case 0x8003:
            SetChrBank2k(CHR_0800, value);
            break;
        case 0xA000:
            SetChrBank1k(CHR_1000, value);
            break;
        case 0xA001:
            SetChrBank1k(CHR_1400, value);
            break;
        case 0xA002:
            SetChrBank1k(CHR_1800, value);
            break;
        case 0xA003:
            SetChrBank1k(CHR_1C00, value);
            break;
        }
    }
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper33::reset()
{
    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}

} //namespace nes
