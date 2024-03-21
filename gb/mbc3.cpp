#include "mbc3.h"
#include <cstdio>

c_mbc3::c_mbc3()
{

}
void c_mbc3::write_byte(uint16_t address, uint8_t data)
{
	//printf("mbc3 write: %04X <- %02X\n", address, data);
	int x;
	switch (address >> 13) {
	case 0: //0000-1FFFF ram enable
		ram_enable = data;
		break;
	case 1: //2000-3FFF rom bank
		bank = data == 0 ? 1 : data & 0x7F;
		bank %= rom_banks;
		break;
	case 2: //4000-5FFF ram bank or rtc register select
		if (data < 8)
			ram_bank = data;
		break;
	case 3: //6000-7FFF latch clock data
		x = 1;
		break;
	case 5: //A000-BFFF RAM
        if (ram_enable == 0x0A)
             //*(ram.get + (((address & 0x1FFF) + ram_bank * 0x2000) % ram_size)) = data;
            ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size] = data;
		break;
	default:
		int x = 1;
		break;
	}
	c_gbmapper::write_byte(address, data);
}
uint8_t c_mbc3::read_byte(uint16_t address)
{
	switch (address >> 13) {
	case 0: //0000-3FFF rom 0
	case 1:
		return c_gbmapper::read_byte(address);
		break;
	case 2: //4000-7FFF rom bank
	case 3:
		return *(rom + (address & 0x3FFF) + bank * 0x4000);
		break;
	case 5: //A000-BFFF ram
        if (ram_enable == 0x0A)
             //return *(ram + (((address & 0x1FFF) + ram_bank * 0x2000) % ram_size));
            return ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size];
		break;
	}
    return 0;
}
void c_mbc3::reset()
{
	ram_bank = 0;
	bank = 1;
	ram_enable = 0;
	rom_banks = (32 << *(rom + 0x148)) / 16;
	c_gbmapper::reset();
}