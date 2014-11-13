#include "mapper79.h"


c_mapper79::c_mapper79()
{
	//Krazy Kreatures
	mapperName = "Mapper 79";
}

c_mapper79::~c_mapper79()
{
}

void c_mapper79::WriteByte(unsigned short address, unsigned char value)
{
	if (address < 0x6000 &&
		(address & 0x4100) == 0x4100)
	{
		SetChrBank8k((value & 0x07) | ((value & 0x40) >> 3));
		SetPrgBank32k((value >> 3) & 0x07);
	}
	else
		c_mapper::WriteByte(address, value);
}
