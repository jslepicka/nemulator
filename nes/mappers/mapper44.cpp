#include "mapper44.h"

namespace nes {

c_mapper44::c_mapper44()
{
    //Super Cool Boy 4-in-1
    //Super Big 7-in-1
    mapperName = "Mapper 44";
}

void c_mapper44::WriteByte(unsigned short address, unsigned char value)
{
    if ((address & 0xE001) == 0xA001)
    {
        writeProtectSram = value & 0x40 ? true : false;
        sram_enabled = value & 0x80;

        int block = value & 0x7;

        if (block > 5)
        {
            prg_mask = 0x1F;
            chr_mask = 0xFF;
            prg_offset = 0x60;
            chr_offset = 0x300;
        }
        else
        {
            prg_mask = 0x0F;
            chr_mask = 0x7F;
            prg_offset = 0x10 * block;
            chr_offset = 0x80 * block;
        }
        Sync();
    }
    else
    {
        c_mapper4::WriteByte(address, value);
    }
}

void c_mapper44::reset()
{
    c_mapper4::reset();
    prg_mask = 0x0F;
    chr_mask = 0x7F;
    Sync();
}

} //namespace nes
