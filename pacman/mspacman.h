#pragma once
#include "pacman.h"

namespace pacman
{
class c_mspacman : public c_pacman
{
  public:
    c_mspacman(PACMAN_MODEL model);
    ~c_mspacman() {};
    int load();
    static const std::vector<load_info_t> load_info;

  private:
    void decrypt_mspacman();
    void decrypt_rom(int src, int dst, int len, std::array<uint8_t, 16> addr_bits);
    uint16_t bitswap(uint16_t in, std::array<uint8_t, 16>);
    void check_mspacman_trap(uint16_t address);
    virtual uint8_t read_byte(uint16_t address);
    virtual void write_byte(uint16_t address, uint8_t data);

    std::unique_ptr<uint8_t[]> prg_rom_overlay; //for ms. pacman
    int use_overlay;
};
} //namespace pacman