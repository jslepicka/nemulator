#include "mapper68.h"


namespace nes {

c_mapper68::c_mapper68()
{
    //After Burner
    mapperName = "Mapper 68";
}

c_mapper68::~c_mapper68()
{
}

void c_mapper68::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        switch (address & 0xF000)
        {
        case 0x8000:
            SetChrBank2k(CHR_0000, value);
            break;
        case 0x9000:
            SetChrBank2k(CHR_0800, value);
            break;
        case 0xA000:
            SetChrBank2k(CHR_1000, value);
            break;
        case 0xB000:
            SetChrBank2k(CHR_1800, value);
            break;
        case 0xC000:
            reg_c = value & 0x7F;
            break;
        case 0xD000:
            reg_d = value & 0x7F;
            break;
        case 0xE000:
            mirroring_mode = value & 0x01;
            nt_mode = value & 0x10;

            if (nt_mode)
            {
                if (mirroring_mode)
                {
                    name_table[0] = name_table[1] = pChrRom + ((reg_c | 0x80) * 0x400);
                    name_table[2] = name_table[3] = pChrRom + ((reg_d | 0x80) * 0x400);
                }
                else
                {
                    name_table[0] = name_table[2] = pChrRom + ((reg_c | 0x80) * 0x400);
                    name_table[1] = name_table[3] = pChrRom + ((reg_d | 0x80) * 0x400);
                }
            }
            else
            {

            if (mirroring_mode)
                set_mirroring(MIRRORING_HORIZONTAL);
            else
                set_mirroring(MIRRORING_VERTICAL);
            }
            break;
        case 0xF000:
            SetPrgBank16k(PRG_8000, value);
            break;
        }
    }
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper68::reset()
{
    mirroring_mode = 0;
    nt_mode = 0;
    reg_c = 0;
    reg_d = 0;
    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}

void c_mapper68::ppu_write(unsigned short address, unsigned char value)
{
    if (nt_mode && (address & 0x3FFF) >= 0x2000 && (address & 0x3FFF) < 0x3F00)
        return;
    else
        c_mapper::ppu_write(address, value);
}

} //namespace nes
