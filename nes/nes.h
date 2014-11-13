///////////////////////////////////////////////////////////////////////////////////
//                                                                               //
//   nemulator (an NES emulator)                                                 //
//                                                                               //
//   Copyright (C) 2003-2009 James Slepicka <james@nemulator.com>                //
//                                                                               //
//   This program is free software; you can redistribute it and/or modify        //
//   it under the terms of the GNU General Public License as published by        //
//   the Free Software Foundation; either version 2 of the License, or           //
//   (at your option) any later version.                                         //
//                                                                               //
//   This program is distributed in the hope that it will be useful,             //
//   but WITHOUT ANY WARRANTY; without even the implied warranty of              //
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               //
//   GNU General Public License for more details.                                //
//                                                                               //
//   You should have received a copy of the GNU General Public License           //
//   along with this program; if not, write to the Free Software                 //
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA   //
//                                                                               //
///////////////////////////////////////////////////////////////////////////////////

#pragma once
//TODO: Remove this.  Only need it for a few defines and macros
#include <windows.h>
#include <map>
#include "..\resampler.h"

class c_cpu;
class c_ppu;
class c_mapper;
class c_joypad;
class c_apu;
class c_apu2;
class c_mem_access_log;
struct iNesHeader;

class c_nes
{
public:
	c_nes(void);
	~c_nes(void);
	int Reset(void);
	int EmulateFrame(void);
	int emulate_frame();
	int emulate_frame_accurate(void);
	int emulate_frame_fast(void);
	int emulation_mode;
	void set_apu_freq(double freq);
	int get_nwc_time();

	enum modes
	{
		EMULATION_MODE_FAST,
		EMULATION_MODE_ACCURATE
	};

	int emulate_frame2();
	int *GetVideo(void);
	int get_sound_buf(const short **sound_buf);
	unsigned char *GetJoy1(void);
	unsigned char *GetJoy2(void);
	unsigned char *GetJoy3(void);
	unsigned char *GetJoy4(void);
	unsigned char *joy1, *joy2, *joy3, *joy4;
	bool loaded;
	unsigned char DmcRead(unsigned short address);
	int Load(/*char *path, char *filename*/);
	char path[MAX_PATH];
	char sram_path[MAX_PATH];
	char filename[MAX_PATH];
	char title[MAX_PATH];
	char pathFile[MAX_PATH];
	char additionalInfo[256];
	const char *get_mapper_name();
	int get_mapper_number();
	int get_mirroring_mode();
	bool played;
	int mmc3_cycles;
	int ppu_cycles;
	void set_sprite_limit(bool limit_sprites);
	bool get_sprite_limit();
	void set_submapper(int submapper);
	int get_crc() {return crc32;};
	c_cpu *cpu;
	c_mapper *mapper;
	c_apu2 *apu2;
	void WriteByte(unsigned short address, unsigned char value);
	unsigned char ReadByte(unsigned short address);
	iNesHeader *header;
	c_mem_access_log *mem_access_log;
	void enable_mixer();
	void disable_mixer();

private:
	static const float NES_AUDIO_RATE;
	c_resampler *resampler;
	int num_apu_samples;
	c_ppu *ppu;

	c_joypad *joypad;

	int crc32;
	int mapperNumber;
	char sramFilename[MAX_PATH];
	unsigned char *OpenSram(void);
	int CloseSram();


	unsigned char *image;
	unsigned char *cpuRam;
	unsigned char *sram;
	int LoadImage(char *pathFile);



	typedef void (c_nes::*line_event)(int);
	void run_cpu(int);
	void run_apu(int);
	void run_mmc3(int);
	void run_sprite0(int);
	void run_start_frame(int);
	void run_vblank_begin(int);
	void run_vblank_end(int);
	void run_end_scanline(int);
	void run_reset_vscroll(int);
	void run_reset_hscroll(int);
	void run_vblank_nmi(int);
	std::multimap<int,line_event> events;

	struct s_event
	{
		int cycle;
		line_event e;
	};

	static const int MAX_EVENTS = 8;

	s_event event_list[MAX_EVENTS];

	int event_index;
	void clear_events();
	void add_event(int cycle, line_event e);


	int do_vblank_nmi;
	int vblank_nmi_delay;
	bool limit_sprites;
};
