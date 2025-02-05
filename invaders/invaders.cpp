#include "invaders.h"
#include <fstream>
#include <vector>
#include <string.h>

import z80;
import crc32;

c_invaders::c_invaders()
{
    system_name = "Arcade";

    display_info.fb_width = 256;
    display_info.fb_height = 224;
    display_info.aspect_ratio = 3.0 / 4.0;
    display_info.rotation = 270;

    rom = std::make_unique<uint8_t[]>(8 * 1024);
    ram = std::make_unique<uint8_t[]>(1 * 1024);
    vram = std::make_unique<uint8_t[]>(7 * 1024);

    z80 = std::make_unique<c_z80>([this](uint16_t address) { return this->read_byte(address); }, //read_byte
                [this](uint16_t address, uint8_t data) { this->write_byte(address, data); }, //write_byte
                [this](uint8_t port) { return this->read_port(port); }, //read_port
                [this](uint8_t port, uint8_t data) { this->write_port(port, data); }, //write_port
                &nmi, &irq, &data_bus);
    loaded = 0;
    reset();
}

c_invaders::~c_invaders()
{
}

int c_invaders::load_romset(std::vector<s_roms> &romset)
{
    for (auto &r : romset) {
        std::ifstream file;
        std::string fn = (std::string)path + (std::string) "/" + r.filename;
        file.open(fn, std::ios_base::in | std::ios_base::binary);
        if (file.is_open()) {
            file.read((char *)(r.loc + r.offset), r.length);
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
    return 1;
}

int c_invaders::load()
{
    // clang-format off
    std::vector<s_roms> invaders_roms = {
        {"invaders.e", 0x14e538b0, 2048, 0x1800, rom.get()},
        {"invaders.f", 0x0ccead96, 2048, 0x1000, rom.get()},
        {"invaders.g", 0x6bfaca4a, 2048, 0x0800, rom.get()},
        {"invaders.h", 0x734f5ad8, 2048, 0x0000, rom.get()},
    };

    // clang-format on

    std::vector<s_roms> romset = invaders_roms;

    if (!load_romset(romset))
        return 0;

    loaded = 1;
    return 0;
}

int c_invaders::is_loaded()
{
    return loaded;
}

int c_invaders::emulate_frame()
{
    for (int line = 0; line < 224; line++) {
        if (line == 95) {
            data_bus = 0xCF; //RST 8
            irq = 1;
        }
        else if (line == 223) {
            data_bus = 0xD7; //RST 10
            irq = 1;
        }
        else {
            irq = 0;
        }
        //this probably isn't accurate -- need to determine exact timing
        z80->execute(149);
    }
    frame_counter++;
    return 0;
}

int c_invaders::reset()
{
    memset(fb, 0, sizeof(fb));
    shift_register = 0;
    shift_amount = 0;
    INP1 = 0x8;
    irq = 0;
    nmi = 0;
    data_bus = 0;
    z80->reset();
    frame_counter = 0;
    return 0;
}

int c_invaders::get_crc()
{
    return 0;
}

void c_invaders::set_input(int input)
{
    INP1 = input;
}

int *c_invaders::get_video()
{
    return (int *)fb;
}

uint8_t c_invaders::read_byte(uint16_t address)
{
    if (address > 0x4000) {
        int check_mirroring = 1;
    }
    if (address < 0x2000) {
        return rom[address];
    }
    else if (address < 0x2400) {
        return ram[address - 0x2000];
    }
    else if (address < 0x4000) {
        return vram[address - 0x2400];
    }
    return 0;
}

void c_invaders::write_byte(uint16_t address, uint8_t data)
{
    if (address < 0x2000) {
        int rom = 1;
    }
    else if (address < 0x2400) {
        ram[address - 0x2000] = data;
    }
    else if (address < 0x4000) {
        int loc = address - 0x2400;
        vram[loc] = data;
        uint32_t *f = &fb[loc * 8];
        for (int i = 0; i < 8; i++) {
            *f++ = (data & 0x1) * 0xffffffff;
            data >>= 1;
        }
    }
    else {
        int x = 1;
    }
}

uint8_t c_invaders::read_port(uint8_t port)
{
    int x;
    switch (port) {
        case 0: //INP0
            break;
        case 1: //INP1
            return INP1;
        case 2: //INP2
            break;
        case 3: //SHFT_IN
            x = 1;
            return ((shift_register << shift_amount) & 0xFF00) >> 8;
        default:
            break;
    }
    return 0;
}

void c_invaders::write_port(uint8_t port, uint8_t data)
{
    int x;
    switch (port) {
        case 2: //SHFTAMNT
            shift_amount = data & 0x7;
            break;
        case 3: //SOUND1
            break;
        case 4: //SHFT_DATA
            shift_register = (shift_register >> 8) | ((uint16_t)data << 8);
            break;
        case 5: //SOUND2
            break;
        case 6: //WATCHDOG
            break;
        default:
            break;
    }
}

int c_invaders::get_sound_bufs(const short **buf_l, const short **buf_r)
{
    //int num_samples = pacman_psg->get_buffer(buf_l);
    *buf_r = NULL;
    return 0;
}
void c_invaders::set_audio_freq(double freq)
{
}

void c_invaders::enable_mixer()
{
}

void c_invaders::disable_mixer()
{
}