#include "mapper32.h"


namespace nes {

c_mapper32::c_mapper32()
{
    //Image Fight (J)
    mapperName = "Mapper 32";
}

void c_mapper32::write_byte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000 && address < 0xC000)
    {
        switch (address & 0xF007)
        {
        case 0x8000:
        case 0x8001:
        case 0x8002:
        case 0x8003:
        case 0x8004:
        case 0x8005:
        case 0x8006:
        case 0x8007:
            prg_reg0 = value;
            sync();
            break;
        case 0x9000:
        case 0x9001:
        case 0x9002:
        case 0x9003:
        case 0x9004:
        case 0x9005:
        case 0x9006:
        case 0x9007:
            prg_mode = value & 0x02;
            if (value & 0x01)
                set_mirroring(MIRRORING_HORIZONTAL);
            else
                set_mirroring(MIRRORING_VERTICAL);
            sync();
            break;
        case 0xA000:
        case 0xA001:
        case 0xA002:
        case 0xA003:
        case 0xA004:
        case 0xA005:
        case 0xA006:
        case 0xA007:
            prg_reg1 = value;
            sync();
            break;
        case 0xB000:
        case 0xB001:
        case 0xB002:
        case 0xB003:
        case 0xB004:
        case 0xB005:
        case 0xB006:
        case 0xB007:
            SetChrBank1k(address & 0x07, value);
            break;
        }
    }
    else
        c_mapper::write_byte(address, value);
}

void c_mapper32::reset()
{
    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    prg_reg0 = 0;
    prg_reg1 = 0;
    prg_mode = 0;
}

void c_mapper32::sync()
{
    SetPrgBank8k(PRG_A000, prg_reg1);
    if (prg_mode)
    {
        SetPrgBank8k(PRG_8000, prgRomPageCount8k - 2);
        SetPrgBank8k(PRG_C000, prg_reg0);
        SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
    }
    else
    {
        SetPrgBank8k(PRG_8000, prg_reg0);
        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    }
}

} //namespace nes
