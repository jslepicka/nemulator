#include "mapper76.h"


namespace nes {

c_mapper76::c_mapper76()
{
    //Digital Devil Monogatari (J)
    mapperName = "Mapper 76";
}

void c_mapper76::write_byte(unsigned short address, unsigned char value)
{
    switch (address & 0xE001)
    {
    case 0x8000:
        prg_mode = value & 0x40;
        command = value & 0x7;
        break;
    case 0x8001:
        switch (command)
        {
        case 2:
        case 3:
        case 4:
        case 5:
            SetChrBank2k((command - 2) * 2, value);
            break;
        case 6:
            if (prg_mode)
                SetPrgBank8k(PRG_C000, value);
            else
                SetPrgBank8k(PRG_8000, value);
            break;
        case 7:
            SetPrgBank8k(PRG_A000, value);
            break;
        default:
            break;
        }
        break;
    case 0xA000:
        if (value & 0x01)
            set_mirroring(MIRRORING_HORIZONTAL);
        else
            set_mirroring(MIRRORING_VERTICAL);
        break;
    default:
        c_mapper::write_byte(address, value);
    }
}

void c_mapper76::sync()
{
}

void c_mapper76::reset()
{
    prg_mode = 0;
    command = 0;
    SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
    SetPrgBank8k(PRG_C000, 0xFE);

}

} //namespace nes
