#pragma once
#include "gbmapper.h"

namespace gb
{

class c_mbc3 : public c_gbmapper
{
  public:
    c_mbc3();
    void write_byte(uint16_t address, uint8_t data);
    uint8_t read_byte(uint16_t address);
    void reset();

  private:
    int bank;
    int ram_enable;
    int ram_bank;
    int rom_banks;
};

} //namespace gb