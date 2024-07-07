#include "mapper190.h"

c_mapper190::c_mapper190()
{
    //Magic Kid GooGoo
    mapperName = "Mapper 190";
    ram = new unsigned char[8192];
}

c_mapper190::~c_mapper190()
{
    delete[] ram;
}

void c_mapper190::WriteByte(unsigned short address, unsigned char value)
{
    switch ((address >> 12) & 0xE)
    {
    case 0x6:
        ram[address - 0x6000] = value;
        break;
    case 0x8:
        SetPrgBank16k(PRG_8000, value & 0x7);
        break;
    case 0xA:
        SetChrBank2k((address & 0x3) << 1, value);
        break;
    case 0xC:
        SetPrgBank16k(PRG_8000, 0x8 | (value & 0x7));
        break;
    default:
        break;
    }
}

unsigned char c_mapper190::ReadByte(unsigned short address)
{
    if (address >= 0x6000 && address < 0x8000)
    {
        return ram[address - 0x6000];
    }
    else
        return c_mapper::ReadByte(address);
}

void c_mapper190::reset()
{
    c_mapper::reset();
    set_mirroring(MIRRORING_VERTICAL);
    SetPrgBank16k(PRG_C000, 0);
    memset(ram, 0, 8192);
}