#include "mapper119.h"


namespace nes {

c_mapper119::c_mapper119()
{
    //High Speed, Pinbot
    mapperName = "TQROM";
    ram = new unsigned char[8192];
}

c_mapper119::~c_mapper119()
{
    delete[] ram;
}

void c_mapper119::WriteChrRom(unsigned short address, unsigned char value)
{
    *(chrBank[(address >> 10) % 8] + (address & 0x3FF)) = value;
}

void c_mapper119::SetChrBank1k(int bank, int value)
{
    if (value & 0x40)
    {
        chrBank[bank] = ram + (((value & 0x3F) % 8) * 0x400);
    }
    else
        c_mapper::SetChrBank1k(bank, (value & 0x3F));
}

} //namespace nes
