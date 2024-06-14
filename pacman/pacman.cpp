#include "pacman.h"
#include "pacman_vid.h"
#include "pacman_psg.h"
#include "../crc32.h"
#include <fstream>

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
    this->model = model;
    reset();
}

c_pacman::~c_pacman()
{
}

int c_pacman::load()
{
    struct s_roms
    {
        uint32_t length;
        uint32_t offset;
        uint32_t crc32;
        std::string filename;
        uint8_t *loc;
    };
    
    s_roms pacman_roms[] = {
        {4096,      0, 0xc1e6ab10, "pacman.6e", prg_rom.get()},
        {4096, 0x1000, 0x1a6fb2d4, "pacman.6f", prg_rom.get()},
        {4096, 0x2000, 0xbcdd1beb, "pacman.6h", prg_rom.get()},
        {4096, 0x3000, 0x817d94e3, "pacman.6j", prg_rom.get()},
        {4096,      0, 0x0c944964, "pacman.5e", pacman_vid->tile_rom.get()},
        {4096,      0, 0x958fedf9, "pacman.5f", pacman_vid->sprite_rom.get()},
        {  32,      0, 0x2fc650bd, "82s123.7f", pacman_vid->color_rom.get()},
        { 256,      0, 0x3eb3a8e4, "82s126.4a", pacman_vid->pal_rom.get()},
        { 256,      0, 0xa9cc86bf, "82s126.1m", pacman_psg->sound_rom},
        { 256,  0x100, 0x77245b66, "82s126.3m", pacman_psg->sound_rom},
    };

    s_roms mspacmab_roms[] = {
        {4096,      0, 0xd16b31b7,     "boot1", prg_rom.get()},
        {4096, 0x1000, 0x0d32de5e,     "boot2", prg_rom.get()},
        {4096, 0x2000, 0x1821ee0b,     "boot3", prg_rom.get()},
        {4096, 0x3000, 0x165a9dd8,     "boot4", prg_rom.get()},
        {4096, 0x8000, 0x8c3e6de6,     "boot5", prg_rom.get()},
        {4096, 0x9000, 0x368cb165,     "boot6", prg_rom.get()},
        {4096,      0, 0x5c281d01,        "5e", pacman_vid->tile_rom.get()},
        {4096,      0, 0x615af909,        "5f", pacman_vid->sprite_rom.get()},
        {  32,      0, 0x2fc650bd, "82s123.7f", pacman_vid->color_rom.get()},
        { 256,      0, 0x3eb3a8e4, "82s126.4a", pacman_vid->pal_rom.get()},
        { 256,      0, 0xa9cc86bf, "82s126.1m", pacman_psg->sound_rom},
        { 256,  0x100, 0x77245b66, "82s126.3m", pacman_psg->sound_rom},
    };

    s_roms *romset;
    int romset_length = 0;

    switch (model) {
        case PACMAN_MODEL::MSPACMAB:
            romset = mspacmab_roms;
            romset_length = sizeof(mspacmab_roms) / sizeof(s_roms);
            prg_mask = 0xFFFF;
            break;
        default:
            romset = pacman_roms;
            romset_length = sizeof(pacman_roms) / sizeof(s_roms);
            prg_mask = 0x7FFF;
            break;
    }

    for (int i = 0; i < romset_length; i++) {
        std::ifstream file;
        std::string fn = (std::string)path + (std::string)"/" + romset[i].filename;
        file.open(fn, std::ios_base::in | std::ios_base::binary);
        if (file.is_open()) {
            file.read((char*)(romset[i].loc + romset[i].offset), romset[i].length);
            file.close();
        }
        else {
            return 0;
        }
        int crc = get_crc32((unsigned char *)(romset[i].loc + romset[i].offset), romset[i].length);
        if (crc != romset[i].crc32) {
            return 0;
        }
        int x = 1;
    }
    loaded = 1;
    //this needs to be done after color roms are loaded
    pacman_vid->build_color_lookup();
    return 0;
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
    address &= prg_mask;
    if (address <= 0x3FFF || address >= 0x8000) {
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