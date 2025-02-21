#include "mapper112.h"


namespace nes {

c_mapper112::c_mapper112()
{
    mapperName = "Mapper 112";
}

void c_mapper112::write_byte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        switch (address & 0xE001)
        {
        case 0x8000:
            command = value & 0x7;
            break;
        case 0x8001:
            {
            int x = 1;
            }
            break;
        case 0xA000:
            switch (command)
            {
            case 0:
            case 1:
                SetPrgBank8k(command, value);
                break;
            case 2:
                SetChrBank2k(CHR_0000, value >> 1);
                break;
            case 3:
                SetChrBank2k(CHR_0800, value >> 1);
                break;
            case 4:
            case 5:
            case 6:
            case 7:
                SetChrBank1k(command, value);
                break;

            }
            break;
        case 0xE000:
            if (value & 0x1)
                set_mirroring(MIRRORING_HORIZONTAL);
            else
                set_mirroring(MIRRORING_VERTICAL);
            break;
        }
    }
}

void c_mapper112::reset()
{
    command = 0;
    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}

} //namespace nes
