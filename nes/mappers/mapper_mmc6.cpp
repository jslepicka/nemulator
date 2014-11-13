#include "mapper_mmc6.h"


c_mapper_mmc6::c_mapper_mmc6()
{
	//Startropics
	mapperName = "MMC6";
}

c_mapper_mmc6::~c_mapper_mmc6()
{
}

void c_mapper_mmc6::reset()
{
	wram_enable = 0;
	wram_control = 0;
	c_mapper4::reset();
}

int c_mapper_mmc6::LoadImage()
{
	header->Rcb1.Sram = true;
	return c_mapper::LoadImage();
}

unsigned char c_mapper_mmc6::ReadByte(unsigned short address)
{
	switch (address >> 12)
	{
	case 6:
		return 0;
	case 7:
		address &= 0x13FF;
		if (((address & 0x200) && (wram_control & 0x80)) ||
			(!(address & 0x200) && (wram_control & 0x20)))
			return sram[address];
		else
			return 0;
	default:
		return c_mapper4::ReadByte(address);
	}
}

void c_mapper_mmc6::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		switch (address & 0xE001)
		{
		case 0x8000:
			commandNumber = value & 0x07;
			prgSelect = value & 0x40 ? true : false;
			prg_xor = prgSelect << 1;
			chrXor = value & 0x80 ? true : false;
			chr_xor = chrXor << 2;
			wram_enable = value & 0x20;
			if (!wram_enable)
				wram_control = 0;
			Sync();
			break;
		case 0xA001:
			if (wram_enable)
				wram_control = value;
			break;
		default:
			c_mapper4::WriteByte(address, value);
			break;
		}
	}
	else if (address >= 0x7000)
	{
		address &= 0x13FF;
		if (((address & 0x200) && (wram_control & 0xC0)) ||
			(!(address & 0x200) && (wram_control & 0x30)))
			sram[address] = value;
	}
}