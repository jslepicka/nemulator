#include "mapper3.h"

c_mapper3::c_mapper3()
{
	mapperName = "CNROM";
}

c_mapper3::~c_mapper3()
{
}

void c_mapper3::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		unsigned char existing_value = ReadByte(address);

		if (existing_value != value)
		{
			int a = 1;
		}
		SetChrBank8k(value & existing_value);
	}
	else
		c_mapper::WriteByte(address, value);

}

void c_mapper3::reset()
{
	c_mapper::reset();
	if (crc32 == 0x6997F5E1)
		this->set_mirroring(MIRRORING_VERTICAL);
}
