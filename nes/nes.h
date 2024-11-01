#pragma once
//TODO: Remove this.  Only need it for a few defines and macros
#include <windows.h>
#include <map>
#include <vector>
#include <functional>
#include "..\console.h"
#include "game_genie.h"
#include <memory>

class c_cpu;
class c_ppu;
class c_mapper;
class c_joypad;
class c_apu;
class c_apu2;
struct iNesHeader;

class c_nes : public c_console
{
public:
    c_nes();
    ~c_nes();
    int reset();
    int emulate_frame();
    void set_audio_freq(double freq);
    int get_nwc_time();
    int is_loaded() { return loaded; }
    void set_input(int input);

    int *get_video();
    int get_sound_bufs(const short **buf_l, const short **buf_r);
    bool loaded;
    unsigned char dmc_read(unsigned short address);
    int load();
    const char *get_mapper_name();
    int get_mapper_number();
    int get_mirroring_mode();
    void set_sprite_limit(bool limit_sprites);
    bool get_sprite_limit();
    void set_submapper(int submapper);
    int get_crc() {return crc32;};
    std::unique_ptr<c_cpu> cpu;
    std::unique_ptr<c_ppu> ppu;
    std::unique_ptr<c_mapper> mapper;
    void write_byte(unsigned short address, unsigned char value);
    unsigned char read_byte(unsigned short address);
    iNesHeader *header;
    void enable_mixer();
    void disable_mixer();

private:
    int LoadImage(char *pathFile);
    std::unique_ptr<c_apu2> apu2;
    std::unique_ptr<c_joypad> joypad;
    std::unique_ptr<c_game_genie> game_genie;
    std::unique_ptr<unsigned char[]> image;
    std::unique_ptr<unsigned char[]> cpuRam;
    std::unique_ptr<unsigned char[]> sram;
    const static std::map<int, std::function<std::unique_ptr<c_mapper>()>> mapper_factory;
    int crc32;
    int mapperNumber;
    int num_apu_samples;
    int file_length;
    char sramFilename[MAX_PATH];
    bool limit_sprites;
};
