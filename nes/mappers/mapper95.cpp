#include "mapper95.h"


namespace nes {

c_mapper95::c_mapper95()
{
    //Dragon Buster (J)
    mapperName = "Mapper 95";
}

void c_mapper95::write_byte(unsigned short address, unsigned char value)
{
    switch (address & 0xE001)
    {
    case 0xA000:
        break;
    default:
        c_mapper4::write_byte(address, value);
    }
}

void c_mapper95::Sync()
{
    int _or = chr_xor >> 1;
    int add = chr_xor >> 2;

    name_table[0] = &vram[0x400 * ((chr[0 | _or] & 0x20) >> 5)];
    name_table[1] = &vram[0x400 * ((chr[(0 | _or) + add] & 0x20) >> 5)];
    name_table[2] = &vram[0x400 * ((chr[1 << _or] & 0x20) >> 5)];
    name_table[3] = &vram[0x400 * ((chr[(1 << _or) + add] & 0x20) >> 5)];
    c_mapper4::Sync();
}

} //namespace nes
