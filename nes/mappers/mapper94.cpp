#include "mapper94.h"


c_mapper94::c_mapper94()
{
	//Senjou no Ookami
	mapperName = "Mapper 94";
}

c_mapper94::~c_mapper94()
{
}

void c_mapper94::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		SetPrgBank16k(PRG_8000, (value & 0x1C) >> 2);

	}
	else
		c_mapper::WriteByte(address, value);
}

void c_mapper94::reset()
{
	SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}

