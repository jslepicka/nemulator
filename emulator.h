#pragma once
#include <Windows.h>
#include <string>
//#include <cstdint>
class i_emulator
{
public:
	i_emulator() {};
	i_emulator(int variant) {}
	virtual ~i_emulator() {}
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
	char path[MAX_PATH];
	char sram_path[MAX_PATH];
	char filename[MAX_PATH];
	char title[MAX_PATH];
	char pathFile[MAX_PATH];
	virtual int get_overscan_color() { return 0xFF000000;	};
};

struct s_emulator_info {
	std::string extension;
	int fb_width;
	int fb_height;
	i_emulator* (*create)(s_emulator_info*);
	void (*destroy)(i_emulator*);
	int type;
	int variant;
	s_emulator_info *next;
};