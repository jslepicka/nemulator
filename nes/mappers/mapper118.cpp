#include "mapper118.h"


c_mapper118::c_mapper118()
{
	//Alien Syndrome
	mapperName = "Mapper 118";
}

void c_mapper118::WriteByte(unsigned short address, unsigned char value)
{

	switch (address & 0xE001)
	{
	case 0xA000:
		break;
	default:
		c_mapper4::WriteByte(address, value);
		break;
	}
}

void c_mapper118::Sync()
{
	int or = chr_xor >> 1;
	int add = chr_xor >> 2;

	name_table[0] = &vram[0x400 * ((chr[0 | or] & 0x80) >> 7)];
	name_table[1] = &vram[0x400 * ((chr[(0 | or) + add] & 0x80) >> 7)];
	name_table[2] = &vram[0x400 * ((chr[1 << or] & 0x80) >> 7)];
	name_table[3] = &vram[0x400 * ((chr[(1 << or) + add] & 0x80) >> 7)];

	c_mapper4::Sync();
}