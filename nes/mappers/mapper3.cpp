#include "mapper3.h"

namespace nes {

c_mapper3::c_mapper3()
{
    mapperName = "CNROM";
}

c_mapper3::~c_mapper3()
{
}

void c_mapper3::write_byte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        unsigned char existing_value = read_byte(address);

        if (existing_value != value)
        {
            int a = 1;
        }
        SetChrBank8k(value & existing_value);
    }
    else
        c_mapper::write_byte(address, value);

}

void c_mapper3::reset()
{
    c_mapper::reset();
}

} //namespace nes
