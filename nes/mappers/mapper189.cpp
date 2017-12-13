#include "mapper189.h"

c_mapper189::c_mapper189(void)
{
	//Thunder Warrior, SF2 World Warrior
	mapperName = "Mapper 189";
}

void c_mapper189::reset(void)
{
	c_mapper4::reset();
	reg_a = 0;
	reg_b = 0;
	Sync();
}

void c_mapper189::WriteByte(unsigned short address, unsigned char value)
{
	if (address < 0x8000 && address >= 0x4120)
	{
		if (address != 0x4132)
		{
			int a = 1;
		}
		reg_a = (value >> 4) | value;
	}
	else if (address >= 0x8000)
	{
		c_mapper4::WriteByte(address, value);
	}
	Sync();
}

void c_mapper189::Sync()
{
	c_mapper4::Sync();
	SetPrgBank32k(reg_a);
}