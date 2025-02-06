#include "invaders.h"
#include <fstream>
#include <vector>
#include <string.h>

import z80;
import crc32;

c_invaders::c_invaders()
{
    system_name = "Arcade";

    display_info.fb_width = FB_WIDTH;
    display_info.fb_height = FB_HEIGHT;
    display_info.aspect_ratio = 3.0 / 4.0;
    display_info.rotation = 270;

    //real hardware uses an i8080, but z80, being (mostly) compatible, seems to work just fine
    z80 = std::make_unique<c_z80>([this](uint16_t address) { return this->read_byte(address); }, //read_byte
                [this](uint16_t address, uint8_t data) { this->write_byte(address, data); }, //write_byte
                [this](uint8_t port) { return this->read_port(port); }, //read_port
                [this](uint8_t port, uint8_t data) { this->write_port(port, data); }, //write_port
                [this]() { this->int_ack(); }, //int_ack callback
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
        {"invaders.e", 0x14e538b0, 2048, 0x1800, rom},
        {"invaders.f", 0x0ccead96, 2048, 0x1000, rom},
        {"invaders.g", 0x6bfaca4a, 2048, 0x0800, rom},
        {"invaders.h", 0x734f5ad8, 2048, 0x0000, rom},
    };

    // clang-format on

    if (!load_romset(invaders_roms))
        return 0;

    loaded = 1;
    return 0;
}

int c_invaders::is_loaded()
{
    return loaded;
}

void c_invaders::int_ack()
{
    //Interrupt flip-flop is cleared when SYNC and D0 of status word are high during
    //i8080 interrupt acknowledge cycle.
    //
    //On Z80, the equivalent is when M1 and IORQ go low during interrupt acknowledge
    //cycle.
    irq = 0;
}

int c_invaders::emulate_frame()
{
    for (int line = 0; line < 262; line++) {
        switch (line) {
            case 96:
                data_bus = 0xCF; //RST 8
                irq = 1;
                break;
            case 224:
                data_bus = 0xD7; //RST 10
                irq = 1;
                break;
            default:
                break;
        }
        //19.968MHz clock divided by 10 / 262 / 59.541985Hz refresh
        z80->execute(128);
    }
    return 0;
}

int c_invaders::reset()
{
    memset(fb, 0, sizeof(fb));
    memset(ram, 0, sizeof(ram));
    memset(vram, 0, sizeof(vram));
    shift_register = 0;
    shift_amount = 0;
    INP1.value = 0x8;
    irq = 0;
    nmi = 0;
    data_bus = 0;
    z80->reset();

    // clang-format off
    INP2 = {
        {
        .ships0 = 1,
        .ships1 = 1,
        .extra_ship = 1,
        }
    };
    // clang-format on

    return 0;
}

int c_invaders::get_crc()
{
    return 0;
}

void c_invaders::set_input(int input)
{
    INP1.value = input;
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
    else {
        int x = 1;
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

        enum screen_loc
        {
            status_line_y = 16,  //line separating playfield from ships/credits info
            base_top_y = 71,     //the top line of the base section
            ufo_bottom_y = 192,  //bottom line of ufo section, just above aliens
            ufo_top_y = 223,     //top line of ufo section, just below scores
            ships_left_x = 25,   //left edge of remaining ships icons
            credits_left_x = 136 //left edge of credits
        };
        int loc = address - 0x2400;
        vram[loc] = data;
        uint32_t *f = &fb[loc * 8];
        int x = loc >> 5; //divide by 32 (256 width / 8 bits/byte)
        int y_base = (loc & 0x1F) * 8;
        const int white = 0xFFFFF0F0;
        const int green = 0xFFB9E000;
        const int orange = 0xFF5A97F1;
        for (int i = 0; i < 8; i++) {
            int color = white;
            int y = y_base + i;
            if (y < screen_loc::status_line_y) {
                if (x >= screen_loc::ships_left_x && x <= screen_loc::credits_left_x) {
                    color = green;
                }
            }
            else if (y > screen_loc::status_line_y && y <= screen_loc::base_top_y) {
                color = green;
            }
            else if (y >= screen_loc::ufo_bottom_y && y <= screen_loc::ufo_top_y) {
                color = orange;
            }
            *f++ = (data & 0x1) * color;
            data >>= 1;
        }
    }
    else {
        int x = 1;
    }
}

uint8_t c_invaders::read_port(uint8_t port)
{
    switch (port) {
        case 0: //INP0
            break;
        case 1: //INP1
            return INP1.value;
        case 2: //INP2
            return INP2.value;
        case 3: //SHFT_IN
            return ((shift_register << shift_amount) & 0xFF00) >> 8;
        default:
            break;
    }
    return 0;
}

void c_invaders::write_port(uint8_t port, uint8_t data)
{
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