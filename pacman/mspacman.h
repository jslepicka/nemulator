#pragma once
#include "pacman.h"

namespace pacman
{
class c_mspacman : public c_pacman, register_system<c_mspacman>
{
  public:
    c_mspacman(PACMAN_MODEL model);
    ~c_mspacman() {};
    int load();
    static std::vector<load_info_t> get_load_info()
    {
        return {
            {
                .game_type = GAME_MSPACMAN,
                .is_arcade = 1,
                .identifier = "mspacman",
                .title = "Ms. Pac-Man",
                .constructor = []() { return new c_mspacman(PACMAN_MODEL::MSPACMAN); },
            },
            {
                .game_type = GAME_MSPACMNF,
                .is_arcade = 1,
                .identifier = "mspacmnf",
                .title = "Ms. Pac-Man (Fast)",
                .constructor = []() { return new c_mspacman(PACMAN_MODEL::MSPACMNF); },
            },
        };
    }

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