#include "mbc2.h"

c_mbc2::c_mbc2()
{

}

void c_mbc2::write_byte(uint16_t address, uint8_t data)
{
	int x;
	switch (address >> 12) {
	case 0:
	case 1: //0000-1FFF ram enable
		if (address & 0x0100) //least significant bit of upper address byte must be zero
			return;
		ram_enable = data;
		break;
	case 2:
	case 3: //2000-3FFF rom bank
		if (!(address & 0x0100)) //least significant bit of upper address byte must be one
			return;
		bank = data & 0x1F;
		if (bank == 0)
			bank = 1;
		break;
	case 0xA:
		if (address < 0xA200) {
			if (ram && (ram_enable & 0xF) == 0xA)
				*(ram + (address & 0x1FF)) = data & 0xF;
		}
        break;
	default:
		int x = 1;
		break;
	}
}
uint8_t c_mbc2::read_byte(uint16_t address)
{
	int a;
	switch (address >> 12) {
	case 0: //0000-3FFF rom 0
	case 1:
	case 2:
	case 3:
		return c_gbmapper::read_byte(address);
		break;
	case 4: //4000-7FFF rom bank
	case 5:
	case 6:
	case 7:
		a = (address & 0x3FFF);
		a += bank * 0x4000;
		return *(rom + a);
		break;
	case 0xA: //A000-BFFF ram
		if (address < 0xA200) {
			if (ram && (ram_enable & 0xF) == 0xA)
				return *(ram + (address & 0x1FF)) & 0xF;
		}
		return 0;
		break;
	}
	return 0;
}
void c_mbc2::reset()
{
	bank = 0;
	ram_enable = 0;
	mode = 0;
	ram_bank = 0;
	rom_banks = (32 << *(rom + 0x148)) / 16;
	c_gbmapper::reset();
}