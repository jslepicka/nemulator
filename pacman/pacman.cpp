#include "pacman.h"
#include "pacman_vid.h"
#include "pacman_psg.h"
#include <fstream>
#include <vector>
#include <string.h>

import z80;
import crc32;

c_pacman::c_pacman(PACMAN_MODEL model)
{
    system_name = "Arcade";

    display_info.fb_width = 288;
    display_info.fb_height = 224;
    display_info.aspect_ratio = 3.0 / 4.0;
    display_info.rotated = true;

    prg_rom = std::make_unique<uint8_t[]>(64 * 1024);
    work_ram = std::make_unique<uint8_t[]>(1 * 1024);

       z80 = std::make_unique<c_z80>([this](uint16_t address) { return this->read_byte(address); }, //read_byte
                    [this](uint16_t address, uint8_t data) { this->write_byte(address, data); }, //write_byte
                    [this](uint8_t port) { return this->read_port(port); }, //read_port
                    [this](uint8_t port, uint8_t data) { this->write_port(port, data); }, //write_port
                    &nmi, &irq, &data_bus);
    pacman_vid = std::make_unique<c_pacman_vid>(this, &irq);
    pacman_psg = std::make_unique<c_pacman_psg>();
    loaded = 0;
    use_decrypted = 0;
    this->model = model;
    if (model == PACMAN_MODEL::MSPACMAN || model == PACMAN_MODEL::MSPACMNF) {
        is_mspacman = 1;
        decrypted_rom = std::make_unique<uint8_t[]>(64 * 1024);
    }
    else {
        is_mspacman = 0;
    }
    reset();
}

c_pacman::~c_pacman()
{
}

int c_pacman::load()
{
    struct s_roms
    {
        std::string filename;
        uint32_t crc32;
        uint32_t length;
        uint32_t offset;
        uint8_t *loc;
    };
    std::vector<s_roms> pacman_roms = {
        {"pacman.6e", 0xc1e6ab10, 4096,      0, prg_rom.get()},
        {"pacman.6f", 0x1a6fb2d4, 4096, 0x1000, prg_rom.get()},
        {"pacman.6h", 0xbcdd1beb, 4096, 0x2000, prg_rom.get()},
        {"pacman.6j", 0x817d94e3, 4096, 0x3000, prg_rom.get()},
        {"pacman.5e", 0x0c944964, 4096,      0, pacman_vid->tile_rom.get()},
        {"pacman.5f", 0x958fedf9, 4096,      0, pacman_vid->sprite_rom.get()},
        {"82s123.7f", 0x2fc650bd,   32,      0, pacman_vid->color_rom.get()},
        {"82s126.4a", 0x3eb3a8e4,  256,      0, pacman_vid->pal_rom.get()},
        {"82s126.1m", 0xa9cc86bf,  256,      0, pacman_psg->sound_rom},
        {"82s126.3m", 0x77245b66,  256,  0x100, pacman_psg->sound_rom},
    };

    std::vector<s_roms> mspacmab_roms = {
        {    "boot1", 0xd16b31b7, 4096,      0, prg_rom.get()},
        {    "boot2", 0x0d32de5e, 4096, 0x1000, prg_rom.get()},
        {    "boot3", 0x1821ee0b, 4096, 0x2000, prg_rom.get()},
        {    "boot4", 0x165a9dd8, 4096, 0x3000, prg_rom.get()},
        {    "boot5", 0x8c3e6de6, 4096, 0x8000, prg_rom.get()},
        {    "boot6", 0x368cb165, 4096, 0x9000, prg_rom.get()},
        {       "5e", 0x5c281d01, 4096,      0, pacman_vid->tile_rom.get()},
        {       "5f", 0x615af909, 4096,      0, pacman_vid->sprite_rom.get()},
        {"82s123.7f", 0x2fc650bd,   32,      0, pacman_vid->color_rom.get()},
        {"82s126.4a", 0x3eb3a8e4,  256,      0, pacman_vid->pal_rom.get()},
        {"82s126.1m", 0xa9cc86bf,  256,      0, pacman_psg->sound_rom},
        {"82s126.3m", 0x77245b66,  256,  0x100, pacman_psg->sound_rom},
    };

    std::vector<s_roms> mspacman_roms = {
        {"pacman.6e", 0xc1e6ab10, 4096, 0, prg_rom.get()},
        {"pacman.6f", 0x1a6fb2d4, 4096, 0x1000, prg_rom.get()},
        {"pacman.6h", 0xbcdd1beb, 4096, 0x2000, prg_rom.get()},
        {"pacman.6j", 0x817d94e3, 4096, 0x3000, prg_rom.get()},
        {"u5", 0xf45fbbcd, 2048, 0x8000, prg_rom.get()},
        {"u6", 0xa90e7000, 4096, 0x9000, prg_rom.get()},
        {"u7", 0xc82cd714, 4096, 0xb000, prg_rom.get()},

        {"5e", 0x5c281d01, 4096, 0, pacman_vid->tile_rom.get()},
        {"5f", 0x615af909, 4096, 0, pacman_vid->sprite_rom.get()},
        {"82s123.7f", 0x2fc650bd, 32, 0, pacman_vid->color_rom.get()},
        {"82s126.4a", 0x3eb3a8e4, 256, 0, pacman_vid->pal_rom.get()},
        {"82s126.1m", 0xa9cc86bf, 256, 0, pacman_psg->sound_rom},
        {"82s126.3m", 0x77245b66, 256, 0x100, pacman_psg->sound_rom},

    };

    std::vector<s_roms> mspacmnf_roms = {
        {"pacman.6e", 0xc1e6ab10, 4096, 0, prg_rom.get()},
        {"pacfast.6f", 0x720dc3ee, 4096, 0x1000, prg_rom.get()},
        {"pacman.6h", 0xbcdd1beb, 4096, 0x2000, prg_rom.get()},
        {"pacman.6j", 0x817d94e3, 4096, 0x3000, prg_rom.get()},
        {"u5", 0xf45fbbcd, 2048, 0x8000, prg_rom.get()},
        {"u6", 0xa90e7000, 4096, 0x9000, prg_rom.get()},
        {"u7", 0xc82cd714, 4096, 0xb000, prg_rom.get()},

        {"5e", 0x5c281d01, 4096, 0, pacman_vid->tile_rom.get()},
        {"5f", 0x615af909, 4096, 0, pacman_vid->sprite_rom.get()},
        {"82s123.7f", 0x2fc650bd, 32, 0, pacman_vid->color_rom.get()},
        {"82s126.4a", 0x3eb3a8e4, 256, 0, pacman_vid->pal_rom.get()},
        {"82s126.1m", 0xa9cc86bf, 256, 0, pacman_psg->sound_rom},
        {"82s126.3m", 0x77245b66, 256, 0x100, pacman_psg->sound_rom},

    };

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
        case PACMAN_MODEL::MSPACMAB:
            romset = mspacmab_roms;
            prg_mask = 0xFFFF;
            break;
        default:
            romset = pacman_roms;
            prg_mask = 0x7FFF;
            break;
    }

    for (auto &r : romset) {
        std::ifstream file;
        std::string fn = (std::string)path + (std::string)"/" + r.filename;
        file.open(fn, std::ios_base::in | std::ios_base::binary);
        if (file.is_open()) {
            file.read((char*)(r.loc + r.offset), r.length);
            file.close();
        }
        else {
            return 0;
        }
        int crc = get_crc32((unsigned char *)(r.loc + r.offset), r.length);
        if (crc != r.crc32) {
            return 0;
        }
        int x = 1;
    }
    if (is_mspacman) {
        decrypt_mspacman();
    }
    loaded = 1;
    //this needs to be done after color roms are loaded
    pacman_vid->build_color_lookup();
    return 0;
}

uint16_t c_pacman::bitswap(uint16_t in, std::array<uint8_t, 16> pos)
{
    uint16_t out = 0;
    for (int i = 0; i < 16; i++) {
        out <<= 1;
        out |= ((in >> pos[i]) & 0x1);
    }
    return out;
}

void c_pacman::decrypt_rom(int src, int dst, int len, std::array<uint8_t, 16> addr_bits)
{
    for (int i = 0; i < len; i++) {
        uint8_t encrypted_val = prg_rom[src + bitswap(i, addr_bits)];
        uint8_t val = bitswap(encrypted_val, {15, 14, 13, 12, 11, 10, 9, 8, 0, 4, 5, 7, 6, 3, 2, 1});
        decrypted_rom[dst + i] = val;
    }
}

int c_pacman::decrypt_mspacman()
{
    //copy pacman 6e, 6f, and 6h into decrypted rom
    memcpy(decrypted_rom.get(), prg_rom.get(), 0x3000);
    
    //copy portion of 6f into decrypted rom
    memcpy(decrypted_rom.get() + 0x9800, prg_rom.get() + 0x1800, 0x800);

    //mirror 6h and 6j into high rom
    memcpy(decrypted_rom.get() + 0xA000, prg_rom.get() + 0x2000, 0x1000);
    memcpy(decrypted_rom.get() + 0xB000, prg_rom.get() + 0x3000, 0x1000);

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
            decrypted_rom[i + p.dst] = decrypted_rom[i + p.src];
        }
    }

    return 1;
}

int c_pacman::is_loaded()
{
    return loaded;
}

int c_pacman::emulate_frame()
{
    pacman_psg->clear_buffer();
    for (int line = 0; line < 264; line++) {
        pacman_vid->execute(384);
        z80->execute(192);
        pacman_psg->execute(6);
    }
    frame_counter++;
    return 0;
}

int c_pacman::reset()
{
    irq = 0;
    nmi = 0;
    data_bus = 0;
    z80->reset();
    pacman_vid->reset();
    frame_counter = 0;
    irq_enabled = 0;
    IN0 = 0x9F;
    IN1 = 0xFF;
    return 0;
}

int c_pacman::get_crc()
{
    return 0;
}

void c_pacman::set_input(int input)
{
    IN0 = (~input) & 0x2F |
        0x10 | //rack advance
        0x80;  //credit button
    IN1 = 0xFF;
    
    if (input & 0x80) {
        IN1 &= ~0x20;
    }
}

int *c_pacman::get_video()
{
    return (int*)pacman_vid->fb.get();
}

void c_pacman::check_mspacman_trap(uint16_t address)
{
    switch (address & 0xFFF8) {
        case 0x0038:
        case 0x03b0:
        case 0x1600:
        case 0x2120:
        case 0x3ff0:
        case 0x8000:
        case 0x97f0:
            use_decrypted = 0;
            prg_mask = 0x7FFF;
            break;
        case 0x3ff8:
            use_decrypted = 1;
            prg_mask = 0xFFFF;
        default:
            break;
    }
}

/*
* 0000 0000 0000 0000  0000 prg start
* 0011 1111 1111 1111  3FFF prg end
* 0100 0000 0000 0000  4000 vid ram start
* 0100 0011 1111 1111  43FF vid ram end
* 0100 0100 0000 0000  4400 color ram start
* 0100 0111 1111 1111  47FF color ram end
* 0100 1000 0000 0000  4800 empty start
* 0100 1011 1111 1111  4BFF empty end
* 0100 1100 0000 0000  4C00 work ram start
* 0100 1111 1110 1111  4FEF work ram end
* 0100 1111 1111 0000  4FF0 sprite ram start
* 0100 1111 1111 1111  4FFF sprite ram end
* 0101 0000 0000 0000  5000 io
* 0101 1111 1111 1111  5FFF io end
* 0110 0000 0000 0000  6000
* 0111 0000 0000 0000  7000
*/

/* io range
* 0101 0000 0000 0000  5000 start
* 0101 0000 0000 0111  5007 end
* 0101 0000 0100 0000  5040 start sound
* 0101 0000 0101 1111  505F sound end
* 0101 0000 0110 0000  5060 sprite start
* 0101 0000 0110 1111  506F sprite end
* 0101 0000 0111 0000  5070 unused start
* 0101 0000 0111 1111  507F unused end
* 0101 0000 1000 0000  5080 dip switch
* 0101 0000 1100 0000  50C0 watchdog
*/

uint8_t c_pacman::read_byte(uint16_t address)
{
    if (is_mspacman) {
        check_mspacman_trap(address);
    }
    address &= prg_mask;
    if (address <= 0x3FFF || address >= 0x8000) {

        if (is_mspacman) {
            if (use_decrypted) {
                return decrypted_rom[address];
            }
        }
        return prg_rom[address];
    }
    address &= 0x5FFF;
    if (address <= 0x47FF) {
        return pacman_vid->read_byte(address);
    }
    else if (address >= 0x4C00 && address <= 0x4FEF) {
        return work_ram[address - 0x4C00];
    }
    else if (address <= 0x4FFF) {
        return pacman_vid->read_byte(address);
    }
    else {
        if (address == 0x5000) {
            return IN0;
        }
        else if (address == 0x5040) {
            return IN1;
        }
        else if (address == 0x5080) {
            //dip switch
            return
                1 << 0 | //coins per game 0=free play, 1=1, 2=1 per 2 games, 3=2
                3 << 2 | //lives per game 0=1, 1=2, 2=3, 3=5
                0 << 4 | //bonus for extra life 0=10000, 1=15000, 2=20000, 3=none
                1 << 6 | //difficulty 0=hard, 1=normal
                1 << 7;  //ghost names 0=alternate, 1=normal
        }
    }
    return 0;
}

void c_pacman::write_byte(uint16_t address, uint8_t data)
{
    if (is_mspacman) {
        check_mspacman_trap(address);
    }
    address &= prg_mask;
    if (address <= 0x3FFF || address >= 0x8000) {
        return;
    }
    address &= 0x5FFF;
    if (address <= 0x47FF) {
        pacman_vid->write_byte(address, data);
    }
    else if (address >= 0x4C00 && address <= 0x4FEF) {
        work_ram[address - 0x4C00] = data;
    }
    else if (address <= 0x4FFF) {
        pacman_vid->write_byte(address, data);
    }
    else {
        //io
        address &= 0xFF;
        if (address >= 0x40 && address <= 0x5F) {
            pacman_psg->write_byte(address, data);
        }
        else if (address >= 0x60 && address <= 0x6F) {

            pacman_vid->sprite_locs[address - 0x60] = data;
        }
        else {
            switch (address) {
                case 0x00:
                    if (data) {
                        irq_enabled = 1;
                        set_irq(irq_input);
                    }
                    else {
                        irq_enabled = 0;
                        set_irq(irq_enabled);
                    }
                    break;
                case 0x01:
                    //sound
                    pacman_psg->mute(!data);
                    break;
                case 0x07:
                    //coin counter
                    break;
                case 0xC0:
                    //watchdog reset
                    break;
                default:
                    break;
            }
        }
    }
}

uint8_t c_pacman::read_port(uint8_t port)
{
    return 0;
}

void c_pacman::write_port(uint8_t port, uint8_t data)
{
    if (port == 0x00) {
        data_bus = data;
    }
}

int c_pacman::get_sound_bufs(const short **buf_l, const short **buf_r)
{
    int num_samples = pacman_psg->get_buffer(buf_l);
    *buf_r = NULL;
    return num_samples;
}
void c_pacman::set_audio_freq(double freq)
{
    pacman_psg->set_audio_rate(freq);
}

void c_pacman::set_irq(int irq)
{
    irq_input = irq;
    if (irq_input && irq_enabled) {
        this->irq = 1;
    }
    else {
        this->irq = 0;
    }
}

void c_pacman::enable_mixer()
{
    pacman_psg->enable_mixer();
}

void c_pacman::disable_mixer()
{
    pacman_psg->disable_mixer();
}