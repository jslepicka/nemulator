#pragma once
#include "gbmapper.h"

namespace gb
{

class c_mbc1 : public c_gbmapper
{
  public:
    c_mbc1();
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

} //namespace gb