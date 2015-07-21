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
	resampler = new c_resampler(NES_AUDIO_RATE / 48000.0f);
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

int c_nes::Load()
{
	char sram_path_file[MAX_PATH];
	sprintf_s(sram_path_file, "%s\\%s", sram_path, filename);
	sprintf_s(pathFile, "%s\\%s", path, filename);

	strip_extension(sram_path_file);
	sprintf_s(sramFilename, "%s.ram", sram_path_file);

	strcpy_s(title, filename);

	strip_extension(title);

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

	//if (crc32 == 0xedcf1b71) //solstice needs sprite limiting to prevent glitches in intro
	//{
	//	limit_sprites = true;
	//}

	switch (mapperNumber)
	{
	case 0:
		mapper = new c_mapper();
		break;
	case 1:
		mapper = new c_mapper1();
		break;
	case 2:
		mapper = new c_mapper2();
		break;
	case 3:
		mapper = new c_mapper3();
		break;
	case 4:
		if (crc32 == 0xA80A0F01 || //Incredible Crash Dummies
			//crc32 == 0x018A8699 || //Roger Clemens' MVP Baseball
			crc32 == 0x982DFB38F //|| //Mickey's Safari in Letterland
			//crc32 == 0xAF05F37E || //George Foreman's KO Boxing
			//crc32 == 0x445DD134 //Bart vs. The World
			) 
		{
			mapper = new c_mapper_mc_acc();
		}
		else
		{
			mapper = new c_mapper4();
			if (crc32 == 0x93991433)	//Low G Man
				mapper->set_submapper(1);
		}
		break;
	case 5:
		mapper = new c_mapper5();
		break;
	case 7:
		mapper = new c_mapper7();
		break;
	case 8:
		mapper = new c_mapper8();
		break;
	case 9:
		mapper = new c_mapper9();
		break;
	case 10:
		mapper = new c_mapper10();
		break;
	case 11:
		mapper = new c_mapper11();
		break;
	case 15:
		mapper = new c_mapper15();
		break;
	case 16:
		mapper = new c_mapper16();
		break;
	case 18:
		mapper = new c_mapper18();
		break;
	case 19:
		mapper = new c_mapper19();
		break;
	case 21:
		mapper = new c_mapper_vrc4();
		mapper->set_submapper(1);
		break;
	case 22:
		mapper = new c_mapper_vrc4();
		mapper->set_submapper(3);
		break;
	case 23:
		mapper = new c_mapper_vrc4();
		break;
	case 24:
		mapper = new c_mapper24();
		break;
	case 25:
		mapper = new c_mapper_vrc4();
		mapper->set_submapper(2);
		break;
	case 26:
		mapper = new c_mapper24();
		mapper->set_submapper(1);
		break;
	case 32:
		mapper = new c_mapper32();
		break;
	case 33:
		mapper = new c_mapper33();
		break;
	case 34:
		mapper = new c_mapper34();
		break;
	case 40:
		mapper = new c_mapper40();
		break;
	case 41:
		mapper = new c_mapper41();
		break;
	case 42:
		mapper = new c_mapper42();
		break;
	case 44:
		mapper = new c_mapper44();
		break;
	case 47:
		mapper = new c_mapper47();
		break;
	case 64:
		mapper = new c_mapper64();
		break;
	case 65:
		mapper = new c_mapper65();
		break;
	case 66:
		mapper = new c_mapper66();
		break;
	case 67:
		mapper = new c_mapper67();
		break;
	case 68:
		mapper = new c_mapper68();
		break;
	case 69:
		mapper = new c_mapper69();
		break;
		//No DMC IRQ support -- mapper 71 games are broken
	case 71:
		mapper = new c_mapper71();
		break;
	case 70:
		mapper = new c_mapper70();
		break;
	case 72:
		mapper = new c_mapper72();
		break;
	case 73:
		mapper = new c_mapper73();
		break;
	case 75:
		mapper = new c_mapper75();
		break;
	case 76:
		mapper = new c_mapper76();
		break;
	case 77:
		mapper = new c_mapper77();
		break;
	case 78:
		mapper = new c_mapper78();
		break;
	case 79:
		mapper = new c_mapper79();
		break;
	case 80:
		mapper = new c_mapper80();
		break;
	case 82:
		mapper = new c_mapper82();
		break;
	case 85:
		mapper = new c_mapper85();
		break;
	case 86:
		mapper = new c_mapper86();
		break;
	case 87:
		mapper = new c_mapper87();
		break;
	case 88:
		mapper = new c_mapper88();
		break;
	case 89:
		mapper = new c_mapper89();
		break;
	case 92:
		mapper = new c_mapper92();
		break;
	case 93:
		mapper = new c_mapper93();
		break;
	case 94:
		mapper = new c_mapper94();
		break;
	case 95:
		mapper = new c_mapper95();
		break;
	case 97:
		mapper = new c_mapper97();
		break;
	case 103:
		mapper = new c_mapper103();
		break;
	case 105:
		mapper = new c_mapper105();
		break;
	case 112:
		mapper = new c_mapper112();
		break;
	case 113:
		mapper = new c_mapper113();
		break;
	case 115:
		mapper = new c_mapper115();
		break;
	case 118:
		mapper = new c_mapper118();
		break;
	case 119:
		mapper = new c_mapper119();
		break;
	case 140:
		mapper = new c_mapper140();
		break;
	case 146:
		mapper = new c_mapper146();
		break;
	case 152:
		mapper = new c_mapper152();
		break;
	case 159:
		mapper = new c_mapper16();
		mapper->set_submapper(1);
		break;
	case 180:
		mapper = new c_mapper180();
		break;
	case 184:
		mapper = new c_mapper184();
		break;
	case 185:
		mapper = new c_mapper185();
		break;
	case 193:
		mapper = new c_mapper193();
		break;
	case 220:
		mapper = new c_mapper4();
		break;
	case 228:
		mapper = new c_mapper228();
		break;
	case 232:
		mapper = new c_mapper232();
		break;
	case 243:
		mapper = new c_mapper243();
		break;
	case 0x100:
		mapper = new c_mapper_mmc6();
		break;
		//case 0x101:
		//	mapper = new c_mapper_fds();
		//	break;
	default:
		sprintf_s(additionalInfo, 256, "Unsupported mapper [%d]", mapperNumber);
		return 0;
	}
	strcpy_s(mapper->filename, pathFile);
	strcpy_s(mapper->sramFilename, sramFilename);
	mapper->crc32 = crc32;
	Reset();
	return 1;
}

int c_nes::Reset(void)
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

int *c_nes::GetVideo(void)
{
	return ppu->pFrameBuffer;
}

void c_nes::set_apu_freq(double freq)
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
	int x = resampler->get_sample_buf(&s);
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