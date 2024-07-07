#include "mapper80.h"


c_mapper80::c_mapper80()
{
    mapperName = "Mapper 80";
}

void c_mapper80::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x7000 && address < 0x8000)
    {
        switch (address & 0x7EFF)
        {
        case 0x7EF0: SetChrBank2k(CHR_0000, value >> 1); break;
        case 0x7EF1: SetChrBank2k(CHR_0800, value >> 1); break;
        case 0x7EF2:
        case 0x7EF3:
        case 0x7EF4:
        case 0x7EF5:
            SetChrBank1k(address - 0x7EEE, value);
            break;
        case 0x7EF6:
            if (value & 0x1)
                set_mirroring(MIRRORING_VERTICAL);
            else
                set_mirroring(MIRRORING_HORIZONTAL);
            break;
        case 0x7EFA:
        case 0x7EFB: SetPrgBank8k(PRG_8000, value); break;
        case 0x7EFC:
        case 0x7EFD: SetPrgBank8k(PRG_A000, value); break;
        case 0x7EFE:
        case 0x7EFF: SetPrgBank8k(PRG_C000, value); break;
            break;
        default:
            break;
        }
    }
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper80::reset()
{
    SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
}