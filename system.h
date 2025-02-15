#pragma once
#define MAX_PATH 260
#include <cstdint>
#include <string>
#include "buttons.h"
#include <vector>
#include <functional>
#include "game_types.h"

// An emulated system (game console, arcade machine, etc.)
class c_system
{
public:
    c_system() {};
    virtual ~c_system() {};
    virtual int load() = 0;
    virtual int is_loaded() = 0;
    virtual int emulate_frame() = 0;
    virtual int reset() = 0;
    int get_crc() { return crc32; }
    virtual int get_sound_bufs(const short **buf_l, const short **buf_r) = 0;
    virtual void set_audio_freq(double freq) = 0;
    virtual void set_input(int input) = 0;
    virtual void enable_mixer() {}
    virtual void disable_mixer() {}
    virtual int *get_video() = 0;
    std::vector<s_button_map> *get_button_map() {return &button_map;}
    struct load_info_t
    {
        int game_type;
        int is_arcade = 0;
        //for consoles, identifier is the file extension (e.g., nes)
        //for arcade games, it's the name of the rom set (e.g., pacman)
        std::string identifier;
        std::string title;
        std::function<c_system *()> constructor;
    };
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
        int rotation = 0;
        double aspect_ratio = 4.0 / 3.0;
    };
    //void get_display_info(display_info_t* dst) { memcpy(dst, &display_info, sizeof(display_info_t)); }
    const display_info_t *get_display_info() { return &display_info; }
  protected:
    int crc32 = 0;
    std::string system_name = "Unknown";
    display_info_t display_info;
    std::vector<s_button_map> button_map;
};

class c_system_registry
{
  public:
    static std::vector<std::vector<c_system::load_info_t>> &get_registry()
    {
        static std::vector<std::vector<c_system::load_info_t>> registry;
        return registry;
    }
    static void register_system(std::vector<c_system::load_info_t> li)
    {
        get_registry().push_back(li);
    }
};

template <typename derived>
class register_system
{
    static void _register_system()
    {
        c_system_registry::register_system(derived::get_load_info());
    }

    struct s_registrar
    {
        s_registrar()
        {
            _register_system();
        }
    };
    static inline s_registrar registrar;
};