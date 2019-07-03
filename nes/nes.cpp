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

#include "nes.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "apu2.h"
#include "joypad.h"
//#include "boost/crc.hpp"
//#include "boost/cstdint.hpp"
#include "mappers\mappers.h"
#include "ines.h"
#include "..\mem_access_log.h"
#include "..\crc32.h"

#include <fstream>

#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

extern int mem_viewer_active;

void strip_extension(char *path);

const float c_nes::NES_AUDIO_RATE = 341.0f / 3.0f * 262.0f * 60.0f/* / 3.0f*/;

const float c_nes::g[8] = {
	0.362211048603058f,
	0.221963167190552f,
	0.0642654523253441f,
	0.00309361424297094f,
};
const float c_nes::b2[8] = {
	-1.98811137676239f,
	-1.97556972503662f,
	-1.82084906101227f,
	-1.99095201492310f,
};
const float c_nes::a2[8] = {
	-1.97986423969269f,
	-1.96833491325378f,
	-1.95999586582184f,
	-1.99108636379242f,
};
const float c_nes::a3[8] = {
	0.982510983943939f,
	0.969850599765778f,
	0.960565388202667f,
	0.994418561458588f,
};

const std::map<int, std::function<c_mapper*()> > c_nes::mapper_factory = 
{
	{ 0, []() {return new c_mapper(); } },
	{ 1, []() {return new c_mapper1(); } },
	{ 2, []() {return new c_mapper2(); } },
	{ 3, []() {return new c_mapper3(); } },
	{ 4, []() {return new c_mapper4(); } },
	{ 5, []() {return new c_mapper5(); } },
	{ 7, []() {return new c_mapper7(); } },
	{ 8, []() {return new c_mapper8(); } },
	{ 9, []() {return new c_mapper9(); } },
	{ 10, []() {return new c_mapper10(); } },
	{ 11, []() {return new c_mapper11(); } },
	{ 15, []() {return new c_mapper15(); } },
	{ 16, []() {return new c_mapper16(); } },
	{ 18, []() {return new c_mapper18(); } },
	{ 19, []() {return new c_mapper19(); } },
	{ 21, []() {return new c_mapper_vrc4(1); } },
	{ 22, []() {return new c_mapper_vrc4(3); } },
	{ 23, []() {return new c_mapper_vrc4(); } },
	{ 24, []() {return new c_mapper24(); } },
	{ 25, []() {return new c_mapper_vrc4(2); } },
	{ 26, []() {return new c_mapper24(1); } },
	{ 32, []() {return new c_mapper32(); } },
	{ 33, []() {return new c_mapper33(); } },
	{ 34, []() {return new c_mapper34(); } },
	{ 40, []() {return new c_mapper40(); } },
	{ 41, []() {return new c_mapper41(); } },
	{ 42, []() {return new c_mapper42(); } },
	{ 44, []() {return new c_mapper44(); } },
	{ 47, []() {return new c_mapper47(); } },
	{ 64, []() {return new c_mapper64(); } },
	{ 65, []() {return new c_mapper65(); } },
	{ 66, []() {return new c_mapper66(); } },
	{ 67, []() {return new c_mapper67(); } },
	{ 68, []() {return new c_mapper68(); } },
	{ 69, []() {return new c_mapper69(); } },
	{ 70, []() {return new c_mapper70(); } },
	{ 71, []() {return new c_mapper71(); } },
	{ 72, []() {return new c_mapper72(); } },
	{ 73, []() {return new c_mapper73(); } },
	{ 75, []() {return new c_mapper75(); } },
	{ 76, []() {return new c_mapper76(); } },
	{ 77, []() {return new c_mapper77(); } },
	{ 78, []() {return new c_mapper78(); } },
	{ 79, []() {return new c_mapper79(); } },
	{ 80, []() {return new c_mapper80(); } },
	{ 82, []() {return new c_mapper82(); } },
	{ 85, []() {return new c_mapper85(); } },
	{ 86, []() {return new c_mapper86(); } },
	{ 87, []() {return new c_mapper87(); } },
	{ 88, []() {return new c_mapper88(); } },
	{ 89, []() {return new c_mapper89(); } },
	{ 92, []() {return new c_mapper92(); } },
	{ 93, []() {return new c_mapper93(); } },
	{ 94, []() {return new c_mapper94(); } },
	{ 95, []() {return new c_mapper95(); } },
	{ 97, []() {return new c_mapper97(); } },
	{ 103, []() {return new c_mapper103(); } },
	{ 105, []() {return new c_mapper105(); } },
	{ 112, []() {return new c_mapper112(); } },
	{ 113, []() {return new c_mapper113(); } },
	{ 115, []() {return new c_mapper115(); } },
	{ 118, []() {return new c_mapper118(); } },
	{ 119, []() {return new c_mapper119(); } },
	{ 140, []() {return new c_mapper140(); } },
	{ 146, []() {return new c_mapper146(); } },
	{ 152, []() {return new c_mapper152(); } },
	{ 159, []() {return new c_mapper16(1); } },
	{ 180, []() {return new c_mapper180(); } },
	{ 184, []() {return new c_mapper184(); } },
	{ 185, []() {return new c_mapper185(); } },
	{ 189, []() {return new c_mapper189(); } },
	{ 190, []() {return new c_mapper190(); } },
	{ 193, []() {return new c_mapper193(); } },
	{ 220, []() {return new c_mapper4(); } },
	{ 228, []() {return new c_mapper228(); } },
	{ 232, []() {return new c_mapper232(); } },
	{ 243, []() {return new c_mapper243(); } },
	{ 0x100, []() {return new c_mapper_mmc6(); }},
	{ 0x101, []() {return new c_mapper_mc_acc(); }}

};


c_nes::c_nes(void)
{
	cpuRam = 0;
	sram = 0;
	cpu = 0;
	ppu = 0;
	mapper = 0;
	image = 0;
	joypad = 0;
	loaded = false;
	played = false;
	emulation_mode = EMULATION_MODE_FAST;
	limit_sprites = false;
	mem_access_log = new c_mem_access_log[256*256];
	crc32 = 0;
	resampler = new c_resampler(NES_AUDIO_RATE / 48000.0f, g, b2, a2, a3);
}

c_nes::~c_nes(void)
{
	//CloseSram();
	delete[] mem_access_log;
	if (image)
		delete[] image;
	if (cpuRam)
		delete[] cpuRam;
	if (sram)
		delete[] sram;
	if (cpu)
		delete cpu;
	if (ppu)
		delete ppu;
	if (mapper)
		delete mapper;
	if (apu2)
		delete apu2;
	if (joypad)
		delete joypad;
	if (resampler)
		delete resampler;
}

void c_nes::enable_mixer()
{
	apu2->enable_mixer();
}

void c_nes::disable_mixer()
{
	apu2->disable_mixer();
}

void c_nes::set_sprite_limit(bool limit_sprites)
{
	this->limit_sprites = limit_sprites;
	ppu->limit_sprites = limit_sprites;
}

bool c_nes::get_sprite_limit()
{
	return ppu->limit_sprites;
}

unsigned char c_nes::DmcRead(unsigned short address)
{
	//cpu->availableCycles -= 12;
	cpu->ExecuteApuDMA();
	return ReadByte(address);
}

unsigned char c_nes::ReadByte(unsigned short address)
{
#ifdef MEM_VIEWER
	if (mem_viewer_active)
	{
		mem_access_log[address].timestamp = 1000.0;
		mem_access_log[address].type = 1;
	}
#endif
	switch (address >> 12)
	{
	case 0:
	case 1:
		return cpuRam[address & 0x7FF];
		break;
	case 2:
	case 3:
		//if (address <= 0x2007)
		return ppu->ReadByte(address & 0x2007);
		break;
	case 4:
		switch (address)
		{
		case 0x4015:
			return apu2->read_byte(address);
			break;
		case 0x4016:
		case 0x4017:
			return joypad->ReadByte(address);
			break;
		default:
			break;
		}
		break;
	default:
		return mapper->ReadByte(address);
		break;
	}
	return 0;
}

void c_nes::WriteByte(unsigned short address, unsigned char value)
{
#ifdef MEM_VIEWER
	if (mem_viewer_active)
	{
		mem_access_log[address].timestamp = 1000.0;
		mem_access_log[address].type = 0;
	}
#endif
	switch (address >> 12)
	{
	case 0:
	case 1:
		cpuRam[address & 0x7FF] = value;
		break;
	case 2:
	case 3:
		ppu->WriteByte(address & 0x2007, value);
		mapper->mmc5_ppu_write(address & 0x2007, value);
		break;
	case 4:
		if (address == 0x4014)
		{
			cpu->DoSpriteDMA(ppu->pSpriteMemory, (value & 0xFF) << 8);
		}
		else if (address == 0x4016)
		{
			joypad->WriteByte(address, value);
		}
		else if (address >= 0x4000 && address <= 0x4017)
		{
			//apu->WriteByte(address, value);
			apu2->write_byte(address, value);
		}
		else
			mapper->WriteByte(address, value);
		break;
	default:
		mapper->WriteByte(address, value);
		break;
	}
}

int c_nes::LoadImage(char *pathFile)
{
	std::ifstream file;
	int m = -1;

	file.open(pathFile, std::ios_base::in | std::ios_base::binary);
	if (file.fail())
		return -1;
	file.seekg(0, std::ios_base::end);
	int filelength = (int)file.tellg();
	file.seekg(0, std::ios_base::beg);
	image = new unsigned char[filelength];
	file.read((char*)image, filelength);
	file.close();

	header = (iNesHeader *)image;

	char ines_signature[4] = {'N', 'E', 'S', 0x1a};
	char fds_signature[] = "*NINTENDO-HVC*";

	//if (memcmp(header->Signature, signature, 4) != 0)
	//	return -1;

	if (memcmp(header->Signature, ines_signature, 4) == 0)
	{

		//if expected file size doesn't match real file size, try to fix chr rom page count
		//fixes fire emblem
		int expected_file_size = (header->PrgRomPageCount * 16384) + (header->ChrRomPageCount * 8192) + sizeof(iNesHeader);
		int actual_chr_size = filelength - (header->PrgRomPageCount * 16384) - sizeof(iNesHeader);

		if (filelength != expected_file_size && header->ChrRomPageCount != 0)
		{
			header->ChrRomPageCount = (actual_chr_size / 8192);
		}

		unsigned char *h = (unsigned char*)&header->Rcb2;

		if (*h & 0x0C)
			*h = 0;
		m = (header->Rcb1.mapper_lo) | (header->Rcb2.mapper_hi << 4);
	}
	else if (memcmp(image + 1, fds_signature, 14) == 0)
	{
		//m = 0x101;
		m = -1;
	}

	if (m != -1)
	{
		crc32 = get_crc32((unsigned char*)image + sizeof(iNesHeader), filelength - sizeof(iNesHeader));
	}
	return m;
}

int c_nes::load()
{
	char sram_path_file[MAX_PATH];
	sprintf_s(sram_path_file, "%s\\%s", sram_path, filename);
	sprintf_s(pathFile, "%s\\%s", path, filename);

	strip_extension(sram_path_file);
	sprintf_s(sramFilename, "%s.ram", sram_path_file);

	//strcpy_s(title, filename);

	//strip_extension(title);

	cpu = new c_cpu();
	ppu = new c_ppu();
	joypad = new c_joypad();
	apu2 = new c_apu2();
	apu2->set_nes(this);
	apu2->set_resampler(resampler);

	joy1 = &joypad->joy1;
	joy2 = &joypad->joy2;
	joy2 = &joypad->joy3;
	joy2 = &joypad->joy4;

	mapperNumber = LoadImage(pathFile);

	if (crc32 == 0x96ce586e)
		mapperNumber = 189;

	if (crc32 == 0x889129CB ||	//Startropics (U) [!].nes
		crc32 == 0xD054FFB0)	//Startropics II - Zoda's Revenge (U) [!].nes
	{
		mapperNumber = 0x100;
	}

	if (crc32 == 0x02CC3973)  //Ninja Kid has wrong mapper in header
	{
		mapperNumber = 0x3;
	}

	if (crc32 == 0x9BDE3267)		//Adventures of Dino Riki has bad header
	{
		header->Rcb1.Mirroring = 1;
		mapperNumber = 3;
	}

	if (crc32 == 0x404B2E8B)		//Rad Racer 2 - bad header
	{
		header->Rcb1.Fourscreen = 1;
	}

	if (crc32 == 0x90C773C1) //Goal! Two - bad header
	{
		mapperNumber = 118;
	}

	if (crc32 == 0xA80A0F01 || //Incredible Crash Dummies
		//crc32 == 0x018A8699 || //Roger Clemens' MVP Baseball
		crc32 == 0x982DFB38 //|| //Mickey's Safari in Letterland
		//crc32 == 0xAF05F37E || //George Foreman's KO Boxing
		//crc32 == 0x445DD134 //Bart vs. The World
		)
		mapperNumber = 0x101;

	if (crc32 == 0x6BC65D7E)
		mapperNumber = 140;

	//if (crc32 == 0xedcf1b71) //solstice needs sprite limiting to prevent glitches in intro
	//{
	//	limit_sprites = true;
	//}

	auto m = mapper_factory.find(mapperNumber);
	if (m == mapper_factory.end())
		return 0;
	mapper = (m->second)();

	if (crc32 == 0x93991433)	//Low G Man
		mapper->set_submapper(1);

	strcpy_s(mapper->filename, pathFile);
	strcpy_s(mapper->sramFilename, sramFilename);
	mapper->crc32 = crc32;
	reset();
	return 1;
}

int c_nes::reset(void)
{
	if (!cpuRam)
		cpuRam = new unsigned char[2048];
	memset(cpuRam, 0xFF, 2048);

	mapper->CloseSram();
	cpu->nes = this;
	apu2->reset();
	ppu->Reset();
	ppu->mapper = mapper;
	ppu->cpu = cpu;
	ppu->apu2 = apu2;
	ppu->limit_sprites = limit_sprites;
	mapper->ppu = ppu;
	mapper->cpu = cpu;
	mapper->apu2 = apu2;
	mapper->nes = this;
	mapper->header = header;
	mapper->image = image;
	mapper->LoadImage();
	mapper->reset();
	cpu->reset();
	joypad->Reset();
	loaded = true;
	mmc3_cycles = 260;
	ppu_cycles = 341;
	do_vblank_nmi = false;
	vblank_nmi_delay = 1;
	return 1;
}

void c_nes::run_sprite0(int i)
{
	ppu->set_sprite0_hit();
}

void c_nes::run_mmc3(int i)
{
	mapper->mmc3_check_a12();
}

void c_nes::run_vblank_begin(int i)
{
	do_vblank_nmi = ppu->BeginVBlank();
}

void c_nes::run_vblank_end(int i)
{
	ppu->EndVBlank();
}

void c_nes::run_start_frame(int i)
{
	ppu->StartFrame();
}

void c_nes::run_end_scanline(int i)
{
	ppu->EndScanline();
}

void c_nes::run_reset_vscroll(int i)
{
	ppu->reset_vscroll();
}

void c_nes::run_reset_hscroll(int i)
{
	ppu->reset_hscroll();
}

void c_nes::run_vblank_nmi(int i)
{
	if (do_vblank_nmi)
		cpu->execute_nmi();
}

void c_nes::set_submapper(int submapper)
{
	mapper->set_submapper(submapper);
}

int c_nes::emulate_frame()
{
	switch (emulation_mode)
	{
	case EMULATION_MODE_ACCURATE:
		emulate_frame_accurate();
		break;
	case EMULATION_MODE_FAST:
		emulate_frame_fast();
		break;
	default:
		break;
	}
	return 0;
}

int c_nes::get_nwc_time()
{
	return mapper->get_nwc_time();
}

int c_nes::emulate_frame_accurate()
{
	if (!loaded)
		return 1;
	for (int scanline = 0; scanline < 262; scanline++)
	{
		ppu->eval_sprites();
		ppu->run_ppu(341);
	}
	return 0;
}

void c_nes::clear_events()
{
	for (int i = 0; i < MAX_EVENTS; i++)
		event_list[i].cycle = -1;
	event_index = 0;
}

void c_nes::add_event(int cycle, c_nes::line_event e)
{
	if (event_index < MAX_EVENTS)
	{
		event_list[event_index].cycle = cycle;
		event_list[event_index].e = e;
		event_index++;
	}
}

int c_nes::emulate_frame_fast(void)
{
	if (!loaded)
		return 1;
	for (int scanline = 0; scanline < 262; scanline++)
	{
		clear_events();
		switch (scanline)
		{
		case 0: //0 - 19 are vblank
			add_event(0, &c_nes::run_vblank_begin);
			add_event(vblank_nmi_delay, &c_nes::run_vblank_nmi);
			break;
		case 20: //pre-render
			add_event(0, &c_nes::run_vblank_end);
			add_event(mmc3_cycles, &c_nes::run_mmc3);
			add_event(251, &c_nes::run_reset_vscroll);
			add_event(257, &c_nes::run_reset_hscroll);
			add_event(304, &c_nes::run_start_frame);
			break;
		default:
			if (scanline > 20 && scanline < 261)
			{
				ppu->DrawScanline();
				add_event(251, &c_nes::run_reset_vscroll);
				add_event(257, &c_nes::run_reset_hscroll);
				if (ppu->sprite0_hit_location != -1)
				{
					add_event(ppu->sprite0_hit_location, &c_nes::run_sprite0);
				}
				add_event(mmc3_cycles, &c_nes::run_mmc3);
			}
			break;
		};

		int total_cpu_cycles = 341;
		int total_apu_cycles = 113;
		int executed_cpu_cycles = 0;
		int executed_apu_cycles = 0;

		while (true)
		{
			//get lowest event
			int cycles = 500;
			line_event e = 0;
			int lowest_event;
			for (int i = 0; i < event_index; i++)
			{
				if (event_list[i].cycle != -1 && event_list[i].cycle <= cycles)
				{
					cycles = event_list[i].cycle;
					e = event_list[i].e;
					lowest_event = i;
				}
			}

			if (cycles != 500) //found an event
			{
				event_list[lowest_event].cycle = -1;
				int num_cycles = cycles;

				int apu_cycles = (int)(num_cycles/3);
				int cycles_to_run = apu_cycles - executed_apu_cycles;
				//apu->EndLine(cycles_to_run);
				executed_apu_cycles += cycles_to_run;
				total_apu_cycles -= cycles_to_run;

				cycles_to_run = num_cycles - executed_cpu_cycles;

				cpu->execute_cycles(cycles_to_run);
				apu2->clock(cycles_to_run);
				executed_cpu_cycles += cycles_to_run;
				total_cpu_cycles -= cycles_to_run;

				(this->*e)(scanline);
			}
			else
				break;
		}
		if (total_apu_cycles > 0)
		{
			//apu->EndLine(total_apu_cycles);
			executed_apu_cycles += total_apu_cycles;
		}
		if (total_cpu_cycles > 0)
		{
			cpu->execute_cycles(total_cpu_cycles);
			apu2->clock(total_cpu_cycles);
			executed_cpu_cycles += total_cpu_cycles;
		}
	}
	//apu->EndFrame();
	return 0;
}

int *c_nes::get_video(void)
{
	return ppu->pFrameBuffer;
}

void c_nes::set_audio_freq(double freq)
{
	//apu2->set_frequency(freq);
	resampler->set_m(NES_AUDIO_RATE / freq);
}

unsigned char *c_nes::GetJoy1(void)
{
	return &joypad->joy1;
}

unsigned char *c_nes::GetJoy2(void)
{
	return &joypad->joy2;
}

unsigned char *c_nes::GetJoy3(void)
{
	return &joypad->joy1;
}

unsigned char *c_nes::GetJoy4(void)
{
	return &joypad->joy2;
}

int c_nes::get_sound_buf(const short **sound_buf)
{
	const short *s;
	int x = resampler->get_output_buf(&s);
	*sound_buf = s;
	return x;
}

int c_nes::get_mapper_number()
{
	return mapperNumber;
}

int c_nes::get_mirroring_mode()
{
	if (mapper)
		return mapper->get_mirroring();
	else
		return 0;
}

const char *c_nes::get_mapper_name()
{
	if (mapper)
		return mapper->mapperName;
	else
		return "Unknown mapper";
}

void c_nes::set_input(int input)
{
	joypad->joy1 = input & 0xFF;
	joypad->joy2 = (input >> 8) & 0xFF;
}