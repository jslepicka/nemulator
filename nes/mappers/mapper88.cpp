#include "mapper88.h"


namespace nes {

c_mapper88::c_mapper88()
{
    //Quinty (J)
    //Devil Man (J)
    mapperName = "Mapper 88";
}


void c_mapper88::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        if (address & 0x01)
        {
            switch (command)
            {
            case 0:
                SetChrBank2k(CHR_0000, (value & 0x3F) >> 1);
                break;
            case 1:
                SetChrBank2k(CHR_0800, (value & 0x3F) >> 1);
                break;
            case 2:
                SetChrBank1k(CHR_1000, (value & 0x3F) | 0x40);
                break;
            case 3:
                SetChrBank1k(CHR_1400, (value & 0x3F) | 0x40);
                break;
            case 4:
                SetChrBank1k(CHR_1800, (value & 0x3F) | 0x40);
                break;
            case 5:
                SetChrBank1k(CHR_1C00, (value & 0x3F) | 0x40);
                break;
            case 6:
                SetPrgBank8k(PRG_8000, value & 0x0F);
                break;
            case 7:
                SetPrgBank8k(PRG_A000, value & 0x0F);
                break;
            }
        }
        else
        {
            command = value & 0x7;
            if (submapper == 1)
            {
                if (value & 0x40)
                    set_mirroring(MIRRORING_ONESCREEN_LOW);
                else
                    set_mirroring(MIRRORING_ONESCREEN_HIGH);
            }
        }
    }
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper88::reset()
{
    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    command = 0;

    for (int i = CHR_0000; i <= CHR_1C00; i++)
        SetChrBank1k(i, 0);
}

} //namespace nes
