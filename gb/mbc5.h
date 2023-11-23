#pragma once
#include "gbmapper.h"

class c_mbc5 : public c_gbmapper
{
  public:
    c_mbc5();
    void write_byte(uint16_t address, uint8_t data);
    uint8_t read_byte(uint16_t address);
    void reset();

  private:
    int bank_lo;
    int bank_hi;
    int bank;
    int bank_fixup;
    int ram_bank;
    int ram_enable;
    int mode;

    void fixup_bank();
    int rom_banks;
};
