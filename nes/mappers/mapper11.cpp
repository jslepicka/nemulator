#include "mapper11.h"


c_mapper11::c_mapper11(void)
{
	mapperName = "Color Dreams";
}

c_mapper11::~c_mapper11(void)
{
}

void c_mapper11::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		SetPrgBank32k(value & 0x3);
		SetChrBank8k(value >> 4);
	}
	else
		c_mapper::WriteByte(address, value);
}

void c_mapper11::reset(void)
{
	SetPrgBank32k(0);
	//SetChrBank8k(2);
	//c_mapper::Reset();
}