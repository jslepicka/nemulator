#include "mapper228.h"


namespace nes {

c_mapper228::c_mapper228()
{
    //Action 52, Cheetamen II
    mapperName = "Mapper 228";
}

c_mapper228::~c_mapper228()
{
}

void c_mapper228::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        int prg_mode = address & 0x20;
        int prg_page = (address >> 6) & 0x1F;
        const int prg_layout[] = {0, 1, 0, 2};
        int prg_chip = prg_layout[(address >> 11) & 0x03];

        SetChrBank8k(((address & 0x0F) << 2) | value & 0x3);
        if (address & 0x2000)
            set_mirroring(MIRRORING_HORIZONTAL);
        else
            set_mirroring(MIRRORING_VERTICAL);
        if (prg_mode)
        {
            int a = 1;
            //no idea if this is correct.  cheetamen ii and action 52
            //both seem to use the other mode
            int page = prg_page + (prg_chip * 32);
            SetPrgBank16k(PRG_8000, page);
            SetPrgBank16k(PRG_C000, page);
        }
        else
        {
            SetPrgBank32k((prg_page >> 1) + (prg_chip * 16));
        }
    }
    else if (address >= 0x4020 && address < 0x6000)
    {
        regs[address & 0x03] = value & 0x0F;
    }
    else
        c_mapper::WriteByte(address, value);
}

unsigned char c_mapper228::ReadByte(unsigned short address)
{
    if (address >= 0x4020 && address < 0x6000)
        return regs[address & 0x03];
    else
        return c_mapper::ReadByte(address);
}

void c_mapper228::reset()
{
    SetChrBank8k(0);
    for (int i = 0; i < 4; i++)
        regs[i] = 0;
}

} //namespace nes
