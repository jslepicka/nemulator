#include "mapper243.h"


c_mapper243::c_mapper243()
{
    //Sachen games
    //2-in-1 Cosmo Cop
    mapperName = "Mapper 243";
}

void c_mapper243::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x4020 && address < 0x5000)
    {
        switch (address & 0x4101)
        {
        case 0x4100:
            command = value & 0x7;
            break;
        case 0x4101:
            switch (command)
            {
            case 2:
                chr = (chr & 0x7) | ((value & 0x1) << 3);
                sync_chr();
                break;
            case 4:
                chr = (chr & 0xB) | ((value & 0x1) << 2);
                sync_chr();
                break;
            case 5:
                SetPrgBank32k(value & 0x7);
                break;
            case 6:
                chr = (chr & 0xC) | (value & 0x3);
                sync_chr();
                break;
            case 7:
                switch ((value >> 1) & 0x3)
                {
                case 0:
                    set_mirroring(MIRRORING_HORIZONTAL);
                    break;
                case 1:
                    set_mirroring(MIRRORING_VERTICAL);
                    break;
                case 2:
                    set_mirroring(MIRRORING_ONESCREEN_HIGH);
                    name_table[0] = &vram[0];
                    break;
                case 3:
                    set_mirroring(MIRRORING_ONESCREEN_HIGH);
                    break;
                }
                break;
            }
            break;
        }
    }
}

void c_mapper243::reset()
{
    chr = 0;
    command = 0;
}

void c_mapper243::sync_chr()
{
    SetChrBank8k(chr);
}