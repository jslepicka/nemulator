#include "mapper2.h"

c_mapper2::c_mapper2()
{
	mapperName = "UxROM";
}

c_mapper2::~c_mapper2()
{
}

void c_mapper2::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
		SetPrgBank16k(PRG_8000, value);
	else
		c_mapper::WriteByte(address, value);
}

void c_mapper2::reset()
{
	c_mapper::reset();
	if (crc32 == 0x419461d0)
		set_mirroring(MIRRORING_VERTICAL);
	else if (crc32 == 0x9ea1dc76)  //Rainbow Islands has incorrect mirroring
		set_mirroring(MIRRORING_HORIZONTAL);
}