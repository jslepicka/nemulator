#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <tuple>
class c_console
{
public:
    c_console();
    virtual ~c_console();
    virtual int load() = 0;
    virtual int is_loaded() = 0;
    virtual int emulate_frame() = 0;
    virtual int reset() = 0;
    virtual int get_crc() = 0;
    virtual int get_sound_bufs(const short **buf_l, const short **buf_r) = 0;
    virtual void set_audio_freq(double freq) = 0;
    virtual void set_input(int input) = 0;
    virtual void enable_mixer() {}
    virtual void disable_mixer() {}
    virtual void set_emulation_mode(int mode) {}
    virtual int get_emulation_mode() { return 0; }
    virtual int *get_video() = 0;
    virtual void set_sprite_limit(bool limit_sprites) {}
    virtual bool get_sprite_limit() { return true; }
    char path[MAX_PATH];
    char sram_path[MAX_PATH];
    char filename[MAX_PATH];
    char title[MAX_PATH];
    char pathFile[MAX_PATH];
    virtual std::string get_system_name() { return system_name; }
    struct display_info_t
    {
        int fb_width = 0;
        int fb_height = 0;
        int crop_left = 0;
        int crop_right = 0;
        int crop_top = 0;
        int crop_bottom = 0;
        //bool rotated = false;
        int rotation = 0;
        double aspect_ratio = 4.0 / 3.0;

    } display_info;
    void get_display_info(display_info_t* dst) { memcpy(dst, &display_info, sizeof(display_info_t)); }
  protected:
    std::string system_name = "Unknown";
};

