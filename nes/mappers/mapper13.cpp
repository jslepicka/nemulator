#include "mapper13.h"

c_mapper13::c_mapper13()
{
    mapperName = "NES-CPROM";
}

c_mapper13::~c_mapper13()
{
}

void c_mapper13::reset()
{
    //TODO: Videomation has 16k CHR RAM, but mapper.cpp
    //only allocates 8k.  Should generalize that code instead
    //of reimplementing CHR RAM allocation and swapping here.
    if (chrRom != NULL)
    {
        delete[] chrRom;
        chrRom = new chrRomBank[2];
        memset(chrRom, 0, 16384);
        pChrRom = (unsigned char*)chrRom;
        for (int x = CHR_0000; x <= CHR_1C00; x++)
            chrBank[x] = pChrRom + 0x0400 * x;
    }
}

void c_mapper13::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000) {
        unsigned char* base = pChrRom + (value & 0x3) * 0x1000;
        chrBank[CHR_1000] = base;
        chrBank[CHR_1400] = base + 0x400;
        chrBank[CHR_1800] = base + 0x800;
        chrBank[CHR_1C00] = base + 0xC00;
    }
}