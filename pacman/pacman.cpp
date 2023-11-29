#include "pacman.h"
#include "pacman_vid.h"
#include "pacman_psg.h"
#include <fstream>

c_pacman::c_pacman()
{
    system_name = "Arcade";
    prg_rom = new uint8_t[16 * 1024];
    work_ram = new uint8_t[1 * 1024];
    memset(prg_rom, 0, 16 * 1024);
    memset(work_ram, 0, 1 * 1024);

   	z80 = new c_z80([this](uint16_t address) { return this->read_byte(address); }, //read_byte
                    [this](uint16_t address, uint8_t data) { this->write_byte(address, data); }, //write_byte
                    [this](uint8_t port) { return this->read_port(port); }, //read_port
                    [this](uint8_t port, uint8_t data) { this->write_port(port, data); }, //write_port
                    &nmi, &irq, &data_bus);
    pacman_vid = new c_pacman_vid(this, &irq);
    pacman_psg = new c_pacman_psg();
    loaded = 0;
    reset();
}

c_pacman::~c_pacman()
{
    delete[] prg_rom;
    delete[] work_ram;
    delete z80;
    delete pacman_vid;
    delete pacman_psg;
}

int c_pacman::load()
{
    struct
    {
        uint32_t length;
        uint32_t offset;
        std::string filename;
        uint8_t *loc;
    } roms[] = {
        {4096,      0, "pacman.6e", prg_rom},
        {4096, 0x1000, "pacman.6f", prg_rom},
        {4096, 0x2000, "pacman.6h", prg_rom},
        {4096, 0x3000, "pacman.6j", prg_rom},
        {4096,      0, "pacman.5e", pacman_vid->tile_rom},
        {4096,      0, "pacman.5f", pacman_vid->sprite_rom},
        {  32,      0, "82s123.7f", pacman_vid->color_rom},
        { 256,      0, "82s126.4a", pacman_vid->pal_rom},
        { 256,      0, "82s126.1m", pacman_psg->sound_rom},
        { 256,  0x100, "82s126.3m", pacman_psg->sound_rom},
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
    return pacman_vid->fb;
}

uint8_t c_pacman::read_byte(uint16_t address)
{
    address &= 0x7FFF;
    int x = 1;
    if (address <= 0x3FFF) {
        return prg_rom[address];
    }
    else if (address <= 0x47FF) {
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
            return 0x9F;
        }
        else if (address == 0x5040) {
            return IN1;
            return 0xFF;
        }
        else if (address == 0x5080) {
            //dip switch
            uint8_t dip_sw =
                1 << 0 | //coins per game 0=free play, 1=1, 2=1 per 2 games, 3=2
                3 << 2 | //lives per game 0=1, 1=2, 2=3, 3=5
                0 << 4 | //bonus for extra life 0=10000, 1=15000, 2=20000, 3=none
                1 << 6 | //difficulty 0=hard, 1=normal
                1 << 7;  //ghost names 0=alternate, 1=normal
            return dip_sw;

        }
        else {
            int x = 1;
        }
    }
    return 0;
}

void c_pacman::write_byte(uint16_t address, uint8_t data)
{
    uint16_t original_address = address;
    address &= 0x7FFF;
    switch (address >> 14) {
        case 0: {
             //prg rom
            int x = 1;
        } break;
        case 1:
            address &= 0x5FFF;
            if (address >= 0x5000) {
                
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
                                //pacman_vid->irq_enabled = 1;
                                irq_enabled = 1;
                                set_irq(irq_input);
                            }
                            else {
                                //pacman_vid->irq_enabled = 0;
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
                        default: {
                            int x = 1;
                        } break;
                    }
                }
                int x = 1;
                return;
            }
            else if (address >= 0x4C00 && address <= 0x4FEF) {
                //work ram
                int x = 1;
                work_ram[address - 0x4C00] = data;
                return;
            }
            else {
                //video
                int x = 1;
                pacman_vid->write_byte(address, data);
                return;
            }
            break;

    }
    int x = 1;
}

uint8_t c_pacman::read_port(uint8_t port)
{
    int x = 1;
    return 0;
}

void c_pacman::write_port(uint8_t port, uint8_t data)
{
    int x = 1;
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