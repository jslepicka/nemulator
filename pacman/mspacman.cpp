#include "mspacman.h"
#include "pacman_vid.h"
#include "pacman_psg.h"

namespace pacman
{

// clang-format off
const std::vector<c_system::load_info_t> c_mspacman::load_info = {
    {
        .game_type = GAME_MSPACMAN,
        .is_arcade = 1,
        .extension = "mspacman",
        .title = "Ms. Pac-Man",
        .constructor = []() { return new c_mspacman(PACMAN_MODEL::MSPACMAN); },
    },
    {
        .game_type = GAME_MSPACMNF,
        .is_arcade = 1,
        .extension = "mspacmnf",
        .title = "Ms. Pac-Man (Fast)",
        .constructor = []() { return new c_mspacman(PACMAN_MODEL::MSPACMNF); },
    },
};
// clang-format on

c_mspacman::c_mspacman(PACMAN_MODEL model) : c_pacman(model)
{
    prg_rom_overlay = std::make_unique<uint8_t[]>(64 * 1024);
    use_overlay = 0;
}

int c_mspacman::load()
{
    // clang-format off
    std::vector<s_roms> mspacman_roms = {
        {"pacman.6e", 0xc1e6ab10, 4096,      0, prg_rom.get()},
        {"pacman.6f", 0x1a6fb2d4, 4096, 0x1000, prg_rom.get()},
        {"pacman.6h", 0xbcdd1beb, 4096, 0x2000, prg_rom.get()},
        {"pacman.6j", 0x817d94e3, 4096, 0x3000, prg_rom.get()},
        {       "u5", 0xf45fbbcd, 2048, 0x8000, prg_rom.get()},
        {       "u6", 0xa90e7000, 4096, 0x9000, prg_rom.get()},
        {       "u7", 0xc82cd714, 4096, 0xb000, prg_rom.get()},
        {       "5e", 0x5c281d01, 4096,      0, pacman_vid->tile_rom.get()},
        {       "5f", 0x615af909, 4096,      0, pacman_vid->sprite_rom.get()},
        {"82s123.7f", 0x2fc650bd,   32,      0, pacman_vid->color_rom.get()},
        {"82s126.4a", 0x3eb3a8e4,  256,      0, pacman_vid->pal_rom.get()},
        {"82s126.1m", 0xa9cc86bf,  256,      0, pacman_psg->sound_rom},
        {"82s126.3m", 0x77245b66,  256,  0x100, pacman_psg->sound_rom},
    };

    std::vector<s_roms> mspacmnf_roms = {
        { "pacman.6e", 0xc1e6ab10, 4096,      0, prg_rom.get()},
        {"pacfast.6f", 0x720dc3ee, 4096, 0x1000, prg_rom.get()},
        { "pacman.6h", 0xbcdd1beb, 4096, 0x2000, prg_rom.get()},
        { "pacman.6j", 0x817d94e3, 4096, 0x3000, prg_rom.get()},
        {        "u5", 0xf45fbbcd, 2048, 0x8000, prg_rom.get()},
        {        "u6", 0xa90e7000, 4096, 0x9000, prg_rom.get()},
        {        "u7", 0xc82cd714, 4096, 0xb000, prg_rom.get()},
        {        "5e", 0x5c281d01, 4096,      0, pacman_vid->tile_rom.get()},
        {        "5f", 0x615af909, 4096,      0, pacman_vid->sprite_rom.get()},
        { "82s123.7f", 0x2fc650bd,   32,      0, pacman_vid->color_rom.get()},
        { "82s126.4a", 0x3eb3a8e4,  256,      0, pacman_vid->pal_rom.get()},
        { "82s126.1m", 0xa9cc86bf,  256,      0, pacman_psg->sound_rom},
        { "82s126.3m", 0x77245b66,  256,  0x100, pacman_psg->sound_rom},
    };
    // clang-format on

    std::vector<s_roms> romset;

    switch (model) {
        case PACMAN_MODEL::MSPACMAN:
            romset = mspacman_roms;
            prg_mask = 0x7FFF; // change this to 0xFFFF in ms pacman mode
            break;
        case PACMAN_MODEL::MSPACMNF:
            romset = mspacmnf_roms;
            prg_mask = 0x7FFF; // change this to 0xFFFF in ms pacman mode
            break;
        default:
            return 0;
    }

    if (!load_romset(romset))
        return 0;
    decrypt_mspacman();
    loaded = 1;
    pacman_vid->build_color_lookup();
    return 0;
}

uint16_t c_mspacman::bitswap(uint16_t in, std::array<uint8_t, 16> pos)
{
    uint16_t out = 0;
    for (int i = 0; i < 16; i++) {
        out |= (((in >> pos[15 ^ i]) & 0x1) << i);
    }
    return out;
}

void c_mspacman::decrypt_rom(int src, int dst, int len, std::array<uint8_t, 16> addr_bits)
{
    for (int i = 0; i < len; i++) {
        uint8_t encrypted_val = prg_rom[src + bitswap(i, addr_bits)];
        uint8_t val = bitswap(encrypted_val, {15, 14, 13, 12, 11, 10, 9, 8, 0, 4, 5, 7, 6, 3, 2, 1});
        prg_rom_overlay[dst + i] = val;
    }
}

void c_mspacman::decrypt_mspacman()
{
    //copy pacman 6e, 6f, and 6h into decrypted rom
    memcpy(prg_rom_overlay.get(), prg_rom.get(), 0x3000);

    //copy portion of 6f into decrypted rom
    memcpy(prg_rom_overlay.get() + 0x9800, prg_rom.get() + 0x1800, 0x800);

    //mirror 6h and 6j into high rom
    memcpy(prg_rom_overlay.get() + 0xA000, prg_rom.get() + 0x2000, 0x1000);
    memcpy(prg_rom_overlay.get() + 0xB000, prg_rom.get() + 0x3000, 0x1000);

    //decrypt source roms

    std::array<uint8_t, 16> addr_bits = {15, 14, 13, 12, 11, 3, 7, 9, 10, 8, 6, 5, 4, 2, 1, 0};

    //u7
    decrypt_rom(0xB000, 0x3000, 0x1000, addr_bits);

    //u5
    decrypt_rom(0x8000, 0x8000, 0x800, {15, 14, 13, 12, 11, 8, 7, 5, 9, 10, 6, 3, 4, 2, 1, 0});

    //u6
    decrypt_rom(0x9800, 0x8800, 0x800, addr_bits);

    //u6
    decrypt_rom(0x9000, 0x9000, 0x800, addr_bits);

    //copy patches
    struct s_patch
    {
        uint16_t src;
        uint16_t dst;
    };
    std::vector<s_patch> patches = {
        {0x8008, 0x0410}, {0x81D8, 0x08E0}, {0x8118, 0x0A30}, {0x80D8, 0x0BD0}, {0x8120, 0x0C20}, {0x8168, 0x0E58},
        {0x8198, 0x0EA8}, {0x8020, 0x1000}, {0x8010, 0x1008}, {0x8098, 0x1288}, {0x8048, 0x1348}, {0x8088, 0x1688},
        {0x8188, 0x16B0}, {0x80C8, 0x16D8}, {0x81C8, 0x16F8}, {0x80A8, 0x19A8}, {0x81A8, 0x19B8}, {0x8148, 0x2060},
        {0x8018, 0x2108}, {0x81A0, 0x21A0}, {0x80A0, 0x2298}, {0x80E8, 0x23E0}, {0x8000, 0x2418}, {0x8058, 0x2448},
        {0x8140, 0x2470}, {0x8080, 0x2488}, {0x8180, 0x24B0}, {0x80C0, 0x24D8}, {0x81C0, 0x24F8}, {0x8050, 0x2748},
        {0x8090, 0x2780}, {0x8190, 0x27B8}, {0x8028, 0x2800}, {0x8100, 0x2B20}, {0x8110, 0x2B30}, {0x81D0, 0x2BF0},
        {0x80D0, 0x2CC0}, {0x80E0, 0x2CD8}, {0x81E0, 0x2CF0}, {0x8160, 0x2D60}

    };

    for (auto &p : patches) {
        for (int i = 0; i < 8; i++) {
            prg_rom_overlay[i + p.dst] = prg_rom_overlay[i + p.src];
        }
    }
}

void c_mspacman::check_mspacman_trap(uint16_t address)
{
    switch (address & 0xFFF8) {
        case 0x0038:
        case 0x03b0:
        case 0x1600:
        case 0x2120:
        case 0x3ff0:
        case 0x8000:
        case 0x97f0:
            use_overlay = 0;
            prg_mask = 0x7FFF;
            break;
        case 0x3ff8:
            use_overlay = 1;
            prg_mask = 0xFFFF;
            break;
        default:
            break;
    }
}

uint8_t c_mspacman::read_byte(uint16_t address)
{
    check_mspacman_trap(address);
    address &= prg_mask;
    if (address <= 0x3FFF || address >= 0x8000) {
        if (use_overlay) {
            return prg_rom_overlay[address];
        }
        else {
            return prg_rom[address];
        }
    }
    else {
        return c_pacman::read_byte(address);
    }
}

void c_mspacman::write_byte(uint16_t address, uint8_t data)
{
    check_mspacman_trap(address);
    c_pacman::write_byte(address, data);
}

} //namespace pacman