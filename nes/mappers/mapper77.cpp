#include "mapper77.h"


c_mapper77::c_mapper77()
{
    //Napoleon Senki (J) [!].nes
    mapperName = "Mapper 77";
    chr_ram = new unsigned char[8192];
}

c_mapper77::~c_mapper77()
{
    delete[] chr_ram;
    chrRam = false;
}

void c_mapper77::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        SetPrgBank32k(value & 0xF);
        SetChrBank2k(CHR_0000, value >> 4);
    }
    else
        c_mapper::WriteByte(address, value);
}

void c_mapper77::reset()
{
    for (int i = 0; i < 6; i++)
    {
        chrBank[2 + i] = chr_ram + 1024*i;
    }
    chrRam = true;
}