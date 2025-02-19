#include "mapper185.h"


namespace nes {

c_mapper185::c_mapper185()
{
    mapperName = "Mapper 185";
}

c_mapper185::~c_mapper185()
{
}

void c_mapper185::reset()
{
    chr_protected = 0;
    c_mapper::reset();
}

void c_mapper185::write_byte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        if (value & 0x0F && value != 0x13)
            chr_protected = 0;
        else
            chr_protected = 1;
    }
    else
        c_mapper::write_byte(address, value);
}

unsigned char c_mapper185::read_chr(unsigned short address)
{
    if (chr_protected)
        return 0x12;
    else
        return c_mapper::read_chr(address);
}

} //namespace nes
