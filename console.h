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
	virtual int get_sound_bufs(const short** buf_l, const short** buf_r) = 0;
	virtual void set_audio_freq(double freq) = 0;
	virtual void set_input(int input) = 0;
	virtual void enable_mixer() {};
	virtual void disable_mixer() {};
	virtual void set_emulation_mode(int mode) {};
	virtual int get_emulation_mode() { return 0; };
	virtual int *get_video() = 0;
	virtual void set_sprite_limit(bool limit_sprites) {};
	virtual bool get_sprite_limit() { return true; }
	virtual int get_fb_height() = 0;
	virtual int get_fb_width() = 0;
    virtual std::tuple<int, int, int, int> get_crop(){return {0, 0, 0, 0};};
	virtual double get_aspect_ratio() { return 0.0; }
	virtual bool is_rotated() { return false; }
    int get_visible_width()
    {
        auto [l, r, t, b] = get_crop();
        return get_fb_width() - l - r;
    }
    int get_visible_height()
    {
        auto [l, r, t, b] = get_crop();
        return get_fb_height() - t - b;
    }
	char path[MAX_PATH];
	char sram_path[MAX_PATH];
	char filename[MAX_PATH];
	char title[MAX_PATH];
	char pathFile[MAX_PATH];
    virtual std::string get_system_name() { return system_name; };
  protected:
    std::string system_name;
};

