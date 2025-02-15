#include "mapper193.h"

namespace nes {

c_mapper193::c_mapper193()
{
    //Fighting Hero
    mapperName = "Mapper 193";
}

void c_mapper193::reset()
{
    SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
    SetPrgBank8k(PRG_C000, prgRomPageCount8k - 2);
    SetPrgBank8k(PRG_A000, prgRomPageCount8k - 3);
    SetPrgBank8k(PRG_8000, prgRomPageCount8k - 4);
    set_mirroring(MIRRORING_VERTICAL);
}

void c_mapper193::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x6000 && address < 0x8000)
    {
        switch (address & 0x3)
        {
        case 0x0:
            SetChrBank4k(CHR_0000, value >> 2);
            break;
        case 0x1:
            SetChrBank2k(CHR_1000, value >> 1);
            break;
        case 0x2:
            SetChrBank2k(CHR_1800, value >> 1);
            break;
        case 0x3:
            SetPrgBank8k(PRG_8000, value);
            break;

        }
    }
    else
    {
        c_mapper::WriteByte(address, value);
    }
}

} //namespace nes
