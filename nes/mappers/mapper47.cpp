#include "mapper47.h"


c_mapper47::c_mapper47()
{
	//Super Spike V'Ball + Nintendo World Cup
	mapperName = "Mapper 47";
}

void c_mapper47::WriteByte(unsigned short address, unsigned char value)
{
	switch (address >> 12)
	{
	case 0x6:
	case 0x7:
		if (sram_enabled && !writeProtectSram)
		{
			int block = value & 0x1;
			prg_offset = block * 16;
			chr_offset = block * 128;
			Sync();
		}
		break;
	default:
		c_mapper4::WriteByte(address, value);
		break;
	}
}

void c_mapper47::reset()
{
	c_mapper4::reset();
	last_prg_page = 15;
	Sync();
}