#include "mapper66.h"


c_mapper66::c_mapper66(void)
{
	//SMB + Duckhunt
	mapperName = "GxROM";
}

c_mapper66::~c_mapper66(void)
{
}

void c_mapper66::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		SetPrgBank32k((value & 0x30) >> 4);
		SetChrBank8k(value & 0x03);
	}
	else
		c_mapper::WriteByte(address, value);
}

void c_mapper66::reset(void)
{
	SetPrgBank32k(0);
	SetChrBank8k(0);

	if (crc32 == 0xD26EFD78)
		this->set_mirroring(MIRRORING_VERTICAL);
}