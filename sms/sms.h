#pragma once
#include <memory>
#include <string>
#include <vector>
#include "..\buttons.h"

import system;
import class_registry;

class c_z80;

namespace sms
{

class c_vdp;
class c_psg;

enum class SMS_MODEL
{
    SMS,
    GAMEGEAR
};

class c_sms : public c_system, register_class<system_registry, c_sms>
{
  public:
    c_sms(SMS_MODEL model);
    ~c_sms();
    int emulate_frame();
    unsigned char read_byte(unsigned short address);
    unsigned short read_word(unsigned short address);
    void write_byte(unsigned short address, unsigned char value);
    void write_word(unsigned short address, unsigned short value);
    void write_port(int port, unsigned char value);
    unsigned char read_port(int port);
    int reset();
    int *get_video();
    int get_sound_bufs(const short **buf_l, const short **buf_r);
    int irq;
    int nmi;
    void set_audio_freq(double freq);
    int load();
    int is_loaded()
    {
        return loaded;
    }
    void enable_mixer();
    void disable_mixer();
    void set_input(int input);
    SMS_MODEL get_model() const
    {
        return model;
    }

    static std::vector<s_system_info> get_registry_info()
    {
        // clang-format off
        static const std::vector<s_button_map> button_map = {
            {BUTTON_1UP,             0x01},
            {BUTTON_1DOWN,           0x02},
            {BUTTON_1LEFT,           0x04},
            {BUTTON_1RIGHT,          0x08},
            {BUTTON_1B,              0x10}, //button 1
            {BUTTON_1A,              0x20}, //button 2
            {BUTTON_2UP,             0x40},
            {BUTTON_2DOWN,           0x80},
            {BUTTON_2LEFT,          0x100},
            {BUTTON_2RIGHT,         0x200},
            {BUTTON_2B,             0x400}, //button 1
            {BUTTON_2A,             0x800}, //button 2
            {BUTTON_SMS_PAUSE, 0x80000000},
        };
        // clang-format on

        return {
            {
                .name = "Sega Master System",
                .identifier = "sms",
                .display_info = {
                    .fb_width = 256,
                    .fb_height = 192,
                    .crop_top = -14,
                    .crop_bottom = -14,
                },
                .button_map = button_map,
                .constructor = []() { return new c_sms(SMS_MODEL::SMS); },
            },
            {
                .name = "Sega Game Gear",
                .identifier = "gg",
                .display_info = {
                    .fb_width = 256,
                    .fb_height = 192,
                    .crop_left = 48,
                    .crop_right = 48,
                    .crop_top = 24,
                    .crop_bottom = 24,
                },
                .button_map = button_map,
                .constructor = []() { return new c_sms(SMS_MODEL::GAMEGEAR); },
            },
        };
    }


  private:
    SMS_MODEL model;
    int psg_cycles;
    int has_sram = 0;
    int joy = 0xFF;
    int loaded = 0;
    int ram_select;
    int nationalism;
    uint8_t data_bus = 0xFF;
    std::unique_ptr<c_z80> z80;
    std::unique_ptr<c_vdp> vdp;
    std::unique_ptr<c_psg> psg;
    std::unique_ptr<unsigned char[]> ram;
    std::unique_ptr<unsigned char[]> rom;
    int file_length;
    unsigned char *page[3];
    unsigned char cart_ram[16384];
    int load_sram();
    int save_sram();
    void catchup_psg();
    uint64_t last_psg_run;
};

} //namespace sms