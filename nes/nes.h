#pragma once
//TODO: Remove this.  Only need it for a few defines and macros
#include <map>
#include <vector>
#include <functional>
#include "..\system.h"
#include "game_genie.h"
#include "mapper.h"
#include <memory>
#include <string>

namespace nes
{

class c_cpu;
class c_ppu;
class c_mapper;
class c_joypad;
class c_apu;
class c_apu;
struct iNesHeader;

class c_nes : public c_system, register_class<system_registry, c_nes>
{
  public:
    c_nes();
    ~c_nes();
    int reset();
    int emulate_frame();
    void set_audio_freq(double freq);
    int get_nwc_time();
    int is_loaded()
    {
        return loaded;
    }
    void set_input(int input);

    int *get_video();
    int get_sound_bufs(const short **buf_l, const short **buf_r);
    bool loaded;
    unsigned char dmc_read(unsigned short address);
    int load();
    std::string &get_mapper_name();
    int get_mapper_number();
    int get_mirroring_mode();
    void set_sprite_limit(bool limit_sprites);
    bool get_sprite_limit();
    void set_submapper(int submapper);
    std::unique_ptr<c_cpu> cpu;
    std::unique_ptr<c_ppu> ppu;
    std::unique_ptr<c_mapper> mapper;
    void write_byte(unsigned short address, unsigned char value);
    unsigned char read_byte(unsigned short address);
    iNesHeader *header;
    void enable_mixer();
    void disable_mixer();

    static std::vector<s_system_info> get_registry_info()
    {
        // clang-format off
        static const std::vector<s_button_map> button_map = {
            {BUTTON_1A,       0x01},
            {BUTTON_1B,       0x02},
            {BUTTON_1SELECT,  0x04},
            {BUTTON_1START,   0x08},
            {BUTTON_1UP,      0x10},
            {BUTTON_1DOWN,    0x20},
            {BUTTON_1LEFT,    0x40},
            {BUTTON_1RIGHT,   0x80},
            {BUTTON_2A,      0x101},
            {BUTTON_2B,      0x102},
            {BUTTON_2SELECT, 0x104},
            {BUTTON_2START,  0x108},
            {BUTTON_2UP,     0x110},
            {BUTTON_2DOWN,   0x120},
            {BUTTON_2LEFT,   0x140},
            {BUTTON_2RIGHT,  0x180},
        };
        // clang-format on

        static const s_system_info::s_display_info display_info = {
            .fb_width = 256,
            .fb_height = 240,
            .crop_top = 8,
            .crop_bottom = 8,
        };

        return {
            {
                .name = "Nintendo NES",
                .identifier = "nes",
                .display_info = display_info,
                .button_map = button_map,
                .constructor = []() { return new c_nes(); },
            },
            {
                .name = "Nintendo NES",
                .identifier = "nsf",
                .display_info = display_info,
                .button_map = button_map,
                .constructor = []() { return new c_nes(); },
            },
        };
    }

  private:
    int LoadImage(char *pathFile);
    std::unique_ptr<c_apu> apu;
    std::unique_ptr<c_joypad> joypad;
    std::unique_ptr<c_game_genie> game_genie;
    std::unique_ptr<unsigned char[]> image;
    std::unique_ptr<unsigned char[]> ram;
    std::unique_ptr<unsigned char[]> sram;
    int mapperNumber;
    c_mapper::s_mapper_info *mapper_info;
    int file_length;
    char sramFilename[MAX_PATH];
    bool limit_sprites;
};

} //namespace nes