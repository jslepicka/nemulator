#include "sms.h"
#include <fstream>
#include "vdp.h"
#include "psg.h"
#include <stdio.h>
#include "crc.h"

#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

import z80;
import crc32;

namespace sms
{

c_sms::c_sms(SMS_MODEL model)
{
    this->model = model;
    z80 = std::make_unique<c_z80>(
        [this](uint16_t address) { return this->read_byte(address); }, //read_byte
        [this](uint16_t address, uint8_t data) { this->write_byte(address, data); }, //write_byte
        [this](uint8_t port) { return this->read_port(port); }, //read_port
        [this](uint8_t port, uint8_t data) { this->write_port(port, data); }, //write_port
        nullptr, //int_ack callback
        &nmi, &irq, &data_bus);
    vdp = std::make_unique<c_vdp>(this);
    psg = std::make_unique<c_psg>();
    ram = std::make_unique<unsigned char[]>(8192);
}

c_sms::~c_sms()
{
    if (has_sram)
        save_sram();
}

int c_sms::load()
{
    std::ifstream file;
    file.open(path_file, std::ios_base::in | std::ios_base::binary);
    if (file.fail())
        return 0;
    file.seekg(0, std::ios_base::end);
    file_length = (int)file.tellg();
    int has_header = (file_length % 1024) != 0;
    int offset = 0;
    if (has_header) {
        //printf("has header\n");
        offset = file_length - ((file_length / 1024) * 1024);
        file_length = file_length - offset;
    }
    file.seekg(offset, std::ios_base::beg);
    if (file_length == 0 || (file_length & (file_length - 1)) || file_length < 0x2000) {
        return 0;
    }
    int alloc_size = file_length < 0x4000 ? 0x4000 : file_length;
    rom = std::make_unique_for_overwrite<unsigned char[]>(alloc_size);
    file.read((char *)rom.get(), file_length);
    file.close();
    crc32 = get_crc32(rom.get(), file_length);

    for (auto &c : crc_table) {
        if (c.crc == crc32 && c.has_sram) {
            has_sram = 1;
            load_sram();
        }
    }

    if (file_length == 0x2000) {
        memcpy(rom.get() + 0x2000, rom.get(), 0x2000);
    }
    loaded = 1;
    reset();
    return file_length;
}

int c_sms::load_sram()
{
    std::ifstream file;
    file.open(sram_path_file, std::ios_base::in | std::ios_base::binary);
    if (file.fail())
        return 1;

    file.seekg(0, std::ios_base::end);
    int l = (int)file.tellg();

    if (l != 8192)
        return 1;

    file.seekg(0, std::ios_base::beg);

    file.read((char *)cart_ram, 8192);
    file.close();
    return 1;
}

int c_sms::save_sram()
{
    std::ofstream file;
    file.open(sram_path_file, std::ios_base::out | std::ios_base::binary);
    if (file.fail())
        return 1;
    file.write((char *)cart_ram, 8192);
    file.close();
    return 0;
}

int c_sms::reset()
{
    z80->reset();
    vdp->reset();
    psg->reset();
    memset(ram.get(), 0x00, 8192);
    irq = 0;
    nmi = 0;
    page[0] = rom.get();
    page[1] = file_length > 0x4000 ? rom.get() + 0x4000 : rom.get();
    page[2] = file_length > 0x8000 ? rom.get() + 0x8000 : rom.get();
    nationalism = 0;
    ram_select = 0;
    joy = 0xFFFF;
    psg_cycles = 0;
    last_psg_run = 0;
    return 0;
}

int c_sms::emulate_frame()
{
    psg->clear_buffer();
    for (int i = 0; i < 262; i++) {
        z80->execute(228);
        vdp->eval_sprites();
        vdp->draw_scanline();
    }
    catchup_psg();
    return 0;
}

unsigned char c_sms::read_byte(unsigned short address)
{
    if (address < 0xC000) {
        if (address < 0x400) //always read first 1k from rom
        {
            return rom[address];
        }
        else {
            int p = (address >> 14) & 0x3;
            if (p == 2 && (ram_select & 0x8))
                return cart_ram[address & 0x1FFF];
            __assume(p < 3);
            return page[p][address & 0x3FFF];
        }
    }
    else {
        return ram[address & 0x1FFF];
    }
}

unsigned short c_sms::read_word(unsigned short address)
{
    int lo = read_byte(address);
    int hi = read_byte(address + 1);
    return lo | (hi << 8);
}

void c_sms::write_byte(unsigned short address, unsigned char value)
{
    if (address < 0xC000) {
        if (address >= 0x8000 && (ram_select & 0x8)) {
            cart_ram[address & 0x1FFF] = value;
        }
    }
    else {
        //ram
        if (address >= 0xFFFC) {
            //paging registers
            switch (address & 0x3) {
                case 0:
                    ram_select = value;
                    break;
                case 1:
                case 2:
                case 3: {
                    int p = (address & 0x3) - 1;
                    int o = (0x4000 * (value)) % file_length;
                    page[p] = rom.get() + o;
                } break;
            }
        }
        address &= 0x1FFF;
        ram[address] = value;
    }
}

void c_sms::write_word(unsigned short address, unsigned short value)
{
    write_byte(address, value & 0xFF);
    write_byte(address + 1, value >> 8);
}

void c_sms::catchup_psg()
{
    int num_cycles = z80->retired_cycles - last_psg_run;
    last_psg_run = z80->retired_cycles;
    psg->clock(num_cycles);
}

void c_sms::write_port(int port, unsigned char value)
{
    //printf("write %2X to port %2X\n", value, port);
    switch (port >> 6) {
        case 0:
        //printf("Port write to I/O or memory\n");
            if (port == 0x3F) {
            //printf("write %02X to nationalism\n", value);
                nationalism = value;
            }
            break;
        case 1:
        //printf("Port write to PSG\n");
            catchup_psg();
            psg->write(value);
            break;
        case 2:
            switch (port & 0x1) {
                case 0: //VDP data
            //printf("\tVDP data\n");
                    vdp->write_data(value);
                    break;
                case 1: //VDP control
            //printf("\tVDP control\n");
                    vdp->write_control(value);
                    break;
            }
            break;
        case 3:
        //printf("Port write to joypad: %02X\n", port);
            break;
        default:
        //printf("Port write error\n");
            break;
    }
}

unsigned char c_sms::read_port(int port)
{
    switch (port >> 6) {
        case 0:
        //printf("Port read from I/O or memory %2X\n", port);
            if (model == SMS_MODEL::GAMEGEAR) {
                return joy >> 31;
            }
            return 0;
        case 1:
        //printf("Port read from PSG\n");
        //TODO: need to differentiate between even and odd reads to return either h or v vdp counters
            if (port & 0x1) {
                int x = 1;
            }
            return vdp->get_scanline();
        case 2:
        //printf("Port read from VDP\n");
            switch (port & 0x1) {
                case 0: //VDP data
            //printf("Port write to VDP data\n");
                    return vdp->read_data();
                case 1: //VDP control
            //printf("Port write to VDP control\n");
                    return vdp->read_control();
            }
            return 0;
        case 3:
        //printf("Port read from joypad: %02X\n", port);
            if (port == 0xDC) {
                return joy & 0xFF;
            }
            else if (port == 0xDD) {
            //invert and select TH output enable bits
                int out = (nationalism ^ 0xFF) & 0xA;
            //and TH bits with TH values
                out = nationalism & (out << 4);
            //shift bits into position
                out = (out & 0x80) | ((out << 1) & 0x40);
                out = ((joy >> 8) & 0x3F) | out;
                return out;
            }
            return 0xFF;
        default:
        //printf("Port read error\n");
            return 0;
    }
}

void c_sms::set_input(int input)
{
    if (model == SMS_MODEL::SMS)
        nmi = input & 0x8000'0000;
    joy = ~input;
}

int *c_sms::get_video()
{
    return vdp->get_frame_buffer();
}

int c_sms::get_sound_bufs(const short **buf_l, const short **buf_r)
{
    int num_samples = psg->get_buffer(buf_l);
    *buf_r = NULL;
    return num_samples;
}
void c_sms::set_audio_freq(double freq)
{
    psg->set_audio_rate(freq);
}

void c_sms::enable_mixer()
{
    psg->enable_mixer();
}

void c_sms::disable_mixer()
{
    psg->disable_mixer();
}

} //namespace sms