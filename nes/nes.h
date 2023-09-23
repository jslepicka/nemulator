#pragma once
//TODO: Remove this.  Only need it for a few defines and macros
#include <windows.h>
#include <map>
#include <vector>
#include <functional>
#include "..\resampler.h"
#include "..\console.h"
#include "..\biquad4.hpp"
#include "..\biquad.hpp"
#include "game_genie.h"

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
	c_cpu *cpu;
	c_ppu *ppu;
	c_mapper *mapper;
	void write_byte(unsigned short address, unsigned char value);
	unsigned char read_byte(unsigned short address);
	iNesHeader *header;
	void enable_mixer();
	void disable_mixer();
	int get_fb_width() { return 256; }
	int get_fb_height() { return 240; }

private:
	int num_apu_samples;
	c_apu2* apu2;
	c_joypad *joypad;
	c_game_genie* game_genie;
	int crc32;
	int mapperNumber;
	char sramFilename[MAX_PATH];
	unsigned char *image;
	unsigned char *cpuRam;
	unsigned char *sram;
	int LoadImage(char *pathFile);
	bool limit_sprites;
	int file_length;
	const static std::map<int, std::function<c_mapper*()> > mapper_factory;
};
