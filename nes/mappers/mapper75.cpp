#include "mapper75.h"


namespace nes {

c_mapper75::c_mapper75()
{
    //Tetsuwan Atom, Exciting Boxing
    mapperName = "VRC-1";
}

void c_mapper75::reset()
{
    SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
    chr0 = chr1 = 0;
}

void c_mapper75::WriteByte(unsigned short address, unsigned char value)
{
    switch(address >> 12)
    {
    case 0x8:
        SetPrgBank8k(PRG_8000, value & 0xF);
        break;
    case 0x9:
        if (value & 0x1)
            set_mirroring(MIRRORING_HORIZONTAL);
        else
            set_mirroring(MIRRORING_VERTICAL);
        chr0 = (chr0 & 0xF) | ((value & 0x2) << 3);
        chr1 = (chr1 & 0xF) | ((value & 0x4) << 2);
        sync();
        break;
    case 0xA:
        SetPrgBank8k(PRG_A000, value & 0xF);
        break;
    case 0xB:
        break;
    case 0xC:
        SetPrgBank8k(PRG_C000, value & 0xF);
        break;
    case 0xD:
        break;
    case 0xE:
        chr0 = (chr0 & 0x10) | (value & 0xF);
        sync();
        break;
    case 0xF:
        chr1 = (chr1 & 0x10) | (value & 0xF);
        sync();
        break;
    default:
        c_mapper::WriteByte(address, value);
        break;
    }
}

void c_mapper75::sync()
{
    SetChrBank4k(CHR_0000, chr0);
    SetChrBank4k(CHR_1000, chr1);
}

} //namespace nes
