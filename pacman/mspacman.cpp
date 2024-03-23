#include "mspacman.h"
#include "pacman_vid.h"
#include "pacman_psg.h"
#include <fstream>

c_mspacman::c_mspacman()
{
    system_name = "blah";
    prg_rom2 = new uint8_t[16 * 1024];
    memset(prg_rom2, 0, 16 * 1024);
    reset();
}

c_mspacman::~c_mspacman()
{
    delete[] prg_rom2;
}

int c_mspacman::reset()
{
    decode_enabled = 0;
    c_pacman::reset();
    return 0;
}

int c_mspacman::load()
{
    struct
    {
        uint32_t length;
        uint32_t offset;
        std::string filename;
        uint8_t *loc;
    } roms[] = {
        {4096,      0, "pacman.6e", prg_rom.get()},
        {4096, 0x1000, "pacman.6f", prg_rom.get()},
        {4096, 0x2000, "pacman.6h", prg_rom.get()},
        {4096, 0x3000, "pacman.6j", prg_rom.get()},
        {4096,      0,        "5e", pacman_vid->tile_rom.get()},
        {4096,      0,        "5f", pacman_vid->sprite_rom.get()},
        {  32,      0, "82s123.7f", pacman_vid->color_rom.get()},
        { 256,      0, "82s126.4a", pacman_vid->pal_rom.get()},
        { 256,      0, "82s126.1m", pacman_psg->sound_rom},
        { 256,  0x100, "82s126.3m", pacman_psg->sound_rom},
        {2048,      0,        "u5", prg_rom2},
        {4096, 0x1000,        "u6", prg_rom2},
        {4096, 0x3000,        "u7", prg_rom2}
    };

    for (auto &r : roms) {
        std::ifstream file;
        std::string fn = (std::string)path + (std::string)"\\" + r.filename;
        file.open(fn, std::ios_base::in | std::ios_base::binary);
        if (file.is_open()) {
            file.read((char*)(r.loc + r.offset), r.length);
            file.close();
        }
        else {
            return 0;
        }
    }
    loaded = 1;
    return 0;
}

uint8_t c_mspacman::read_byte(uint16_t address)
{
    if (address >= 0x0038 && address <= 0x0038 + 7 || address >= 0x03b0 && address <= 0x03b0 + 7 ||
        address >= 0x1600 && address <= 0x1600 + 7 || address >= 0x2120 && address <= 0x2120 + 7 ||
        address >= 0x3ff0 && address <= 0x3ff0 + 7 || address >= 0x8000 && address <= 0x8000 + 7 ||
        address >= 0x97f0 && address <= 0x97f0 + 7) {
        decode_enabled = 0;
        return c_pacman::read_byte(address);
    }
    else if (address >= 0x3ff8 && address <= 0x3ff8 + 7) {
        decode_enabled = 1;
    }

    if (decode_enabled) {
        if (address >= 0x3000 && address <= 0x3FFF) {
            return prg_rom2[address + 0x3000];
        }
        else if (address >= 0x8000 && address <= 0x97FF) {
            return prg_rom2[address - 0x8000];
        }
    }
    else {
        return c_pacman::read_byte(address);
    }
}

void c_mspacman::write_byte(uint16_t address, uint8_t data)
{
    if (address >= 0x0038 && address <= 0x0038 + 7 || address >= 0x03b0 && address <= 0x03b0 + 7 ||
        address >= 0x1600 && address <= 0x1600 + 7 || address >= 0x2120 && address <= 0x2120 + 7 ||
        address >= 0x3ff0 && address <= 0x3ff0 + 7 || address >= 0x8000 && address <= 0x8000 + 7 ||
        address >= 0x97f0 && address <= 0x97f0 + 7) {
        decode_enabled = 0;
        return;
    }
    c_pacman::write_byte(address, data);
}