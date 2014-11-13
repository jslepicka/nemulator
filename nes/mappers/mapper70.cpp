#include "mapper70.h"


c_mapper70::c_mapper70()
{
	//Family Trainer - Manhattan Police (J)
	//Ge Ge Ge no Kitarou 2 (J)
	//Kamen Rider Club (J)
	mapperName = "Mapper 70";
}

void c_mapper70::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		SetPrgBank16k(PRG_8000, (value & 0x70) >> 4);
		SetChrBank8k(value & 0xF);
		if (value & 0x80)
			set_mirroring(MIRRORING_ONESCREEN_HIGH);
		else
			set_mirroring(MIRRORING_ONESCREEN_LOW);
	}
	else
		c_mapper::WriteByte(address, value);
}

void c_mapper70::reset()
{
	SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
}