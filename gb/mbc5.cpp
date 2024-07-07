#include "mbc5.h"

c_mbc5::c_mbc5()
{
}

void c_mbc5::write_byte(uint16_t address, uint8_t data)
{
    switch (address >> 12) {
        case 0:
        case 1:
            ram_enable = (data & 0x0F) == 0x0A;
            bank = (bank_hi << 8) | bank_lo;
            break;
        case 2:
            bank_lo = data & 0xFF;
            bank = (bank_hi << 8) | bank_lo;
            break;
        case 3:
            bank_hi = data & 0x1;
            bank = (bank_hi << 8) | bank_lo;
            break;
        case 4:
        case 5:
            ram_bank = data & (rumble ? 0xE : 0xF);
            break;
        case 0xA:
        case 0xB:
            if (ram && ram_enable)
                ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size] = data;
            break;
        default: {
            int x = 1;
        }
            break;
    }
}
uint8_t c_mbc5::read_byte(uint16_t address)
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
            a += (bank % rom_banks) * 0x4000;
            return *(rom + a);
            break;
        case 5: //A000-BFFF ram
            if (ram && ram_enable)
                return ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size];
            break;
    }
    return 0;
}
void c_mbc5::reset()
{
    rom_banks = (32 << *(rom + 0x148)) / 16;
    bank = 0;
    bank_fixup = 1;
    ram_enable = 0;
    mode = 0;
    ram_bank = 0;
    bank_lo = 0;
    bank_hi = 0;
    c_gbmapper::reset();
}