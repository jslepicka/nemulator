#include "mbc1.h"

c_mbc1::c_mbc1()
{

}

void c_mbc1::fixup_bank()
{
    bank_fixup = bank;
    if ((bank & 0x1F) == 0 && !(bank & 0x10)) {
        bank_fixup |= 1;
    }
    bank_fixup %= rom_banks;
}

void c_mbc1::write_byte(uint16_t address, uint8_t data)
{
    switch (address >> 13) {
    case 0: //0000-1FFF ram enable
        ram_enable = data;
        break;
    case 1: //2000-3FFF rom bank
        bank_lo = (data & 0x1F);
        bank = (bank & 0x60) | (data & 0x1F);
        fixup_bank();
        break;
    case 2: //4000-5FFF ram bank or upper bits of rom bank number
        bank_hi = data & 0x3;
        if (mode == 0) {
            //rom
            bank = (bank & 0x1F) | ((data & 0x3) << 5);
            fixup_bank();
        }
        else if (mode == 1) {
            ram_bank = data & 0x3;
        }
        break;
    case 3: //6000-7FFF rom/ram mode select
        mode = data & 1;
        break;
    case 5: //A000-BFFF RAM
        if (ram && (ram_enable & 0xF) == 0xA)
             //*(ram + (((address & 0x1FFF) + ram_bank * 0x2000) % ram_size)) = data;
            ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size] = data;
        break;
    default:
        int x = 1;
        break;
    }
    //c_mapper::write_byte(address, data);
}
uint8_t c_mbc1::read_byte(uint16_t address)
{
    int a;
    switch (address >> 13) {
    case 0: //000-3FFF rom 0
    case 1:
        return c_gbmapper::read_byte(address);
        break;
    case 2: //4000-7FFF rom bank
    case 3:
        a = (address & 0x3FFF);
        a += bank_fixup * 0x4000;
        return *(rom + a);
        break;
    case 5: //A000-BFFF ram
        if (ram && (ram_enable & 0xF) == 0xA)
             //return *(ram + (((address & 0x1FFF) + ram_bank * 0x2000) % ram_size));
            return ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size];
        break;
    }
    return 0;
}
void c_mbc1::reset()
{
    bank = 0;
    bank_fixup = 1;
    ram_enable = 0;
    mode = 0;
    ram_bank = 0;
    rom_banks = (32 << *(rom + 0x148)) / 16;
    c_gbmapper::reset();
}