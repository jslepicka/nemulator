#pragma once
#include "pacman.h"

class c_mspacman : public c_pacman
{
  public:
    c_mspacman();
    ~c_mspacman();
    int load();
    virtual int reset();

  private:
    uint8_t read_byte(uint16_t address);
    void write_byte(uint16_t address, uint8_t data);
    uint8_t *prg_rom2;
    int decode_enabled;
};