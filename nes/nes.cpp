#include "nes.h"
#include "cpu.h"
#include "ppu.h"
#include "apu2.h"
#include "joypad.h"
#include "mappers\mappers.h"
#include "ines.h"
#include "..\mem_access_log.h"
#include "..\crc32.h"
#include "cartdb.h"

#include <fstream>

#include <crtdbg.h>
//#if defined(DEBUG) | defined(_DEBUG)
//#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
//#define new DEBUG_NEW
//#endif

extern int mem_viewer_active;

void strip_extension(char *path);

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
	{ 13, []() {return new c_mapper13(); } },
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
	{ 0x101, []() {return new c_mapper_mc_acc(); }},
	{ 0x102, []() {return new c_mapper_nsf(); }}

};


c_nes::c_nes()
{
	cpuRam = 0;
	sram = 0;
	cpu = 0;
	ppu = 0;
	mapper = 0;
	image = 0;
	joypad = 0;
	loaded = false;
	limit_sprites = false;
	crc32 = 0;
}

c_nes::~c_nes()
{
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

unsigned char c_nes::dmc_read(unsigned short address)
{
	//cpu->availableCycles -= 12;
	cpu->execute_apu_dma();
	return read_byte(address);
}

unsigned char c_nes::read_byte(unsigned short address)
{
#ifdef MEM_VIEWER
	if (mem_viewer_active)
	{
		mem_access_log[address].timestamp = 1000.0;
		mem_access_log[address].type = 1;
	}
#endif
	//unlimited health in holy diver
	//if (address == 0x0440) {
	//	return 6;
	//}
	//start battletoads on level 2
	//if (address == 0x8320) {
	//	return 0x2;
	//}
	switch (address >> 12)
	{
	case 0:
	case 1:
		return cpuRam[address & 0x7FF];
		break;
	case 2:
	case 3:
		//if (address <= 0x2007)
		return ppu->read_byte(address & 0x2007);
		break;
	case 4:
		switch (address)
		{
		case 0x4015:
			return apu2->read_byte(address);
			break;
		case 0x4016:
		case 0x4017:
			return joypad->read_byte(address);
			break;
		default:
			break;
		}
		break;
	default:
		unsigned char val = mapper->ReadByte(address);
		return val;
		break;
	}
	return 0;
}

void c_nes::write_byte(unsigned short address, unsigned char value)
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
		ppu->write_byte(address & 0x2007, value);
		mapper->mmc5_ppu_write(address & 0x2007, value);
		break;
	case 4:
		if (address == 0x4014)
		{
			cpu->do_sprite_dma(ppu->pSpriteMemory, (value & 0xFF) << 8);
		}
		else if (address == 0x4016)
		{
			joypad->write_byte(address, value);
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
	file_length = (int)file.tellg();
	file.seekg(0, std::ios_base::beg);
	image = new unsigned char[file_length];
	file.read((char*)image, file_length);
	file.close();

	header = (iNesHeader *)image;

	char ines_signature[4] = {'N', 'E', 'S', 0x1a};
	char fds_signature[] = "*NINTENDO-HVC*";
	char nsf_signature[] = { 'N', 'E', 'S', 'M', 0x1A };

	//if (memcmp(header->Signature, signature, 4) != 0)
	//	return -1;

	if (memcmp(header->Signature, ines_signature, 4) == 0)
	{

		//if expected file size doesn't match real file size, try to fix chr rom page count
		//fixes fire emblem
		int expected_file_size = (header->PrgRomPageCount * 16384) + (header->ChrRomPageCount * 8192) + sizeof(iNesHeader);
		int actual_chr_size = file_length - (header->PrgRomPageCount * 16384) - sizeof(iNesHeader);

		if (file_length != expected_file_size && header->ChrRomPageCount != 0)
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
	else if (memcmp(image, nsf_signature, 5) == 0) {
		m = 0x102;
	}

	if (m != -1 && m != 0x102)
	{
		crc32 = get_crc32((unsigned char*)image + sizeof(iNesHeader), file_length - sizeof(iNesHeader));
	}
	return m;
}

int c_nes::load()
{
	char sram_path_file[MAX_PATH];
	int submapper = -1;
	sprintf_s(sram_path_file, "%s\\%s", sram_path, filename);
	sprintf_s(pathFile, "%s\\%s", path, filename);

	strip_extension(sram_path_file);
	sprintf_s(sramFilename, "%s.ram", sram_path_file);

	cpu = new c_cpu();
	ppu = new c_ppu();
	joypad = new c_joypad();
	apu2 = new c_apu2();
	apu2->set_nes(this);

	mapperNumber = LoadImage(pathFile);

	auto cartdb_entry = cartdb.find(crc32);
	if (cartdb_entry != cartdb.end()) {
		s_cartdb c = cartdb_entry->second;
		if (c.mapper != -1) {
			mapperNumber = c.mapper;
		}
		if (c.submapper != -1) {
			submapper = c.submapper;
		}
		if (c.mirroring != -1) {
			switch (c.mirroring) {
			case 0:
			case 1:
				header->Rcb1.Mirroring = c.mirroring;
				break;
			case 4:
				header->Rcb1.Fourscreen = 1;
			}
		}
	}

	auto m = mapper_factory.find(mapperNumber);
	if (m == mapper_factory.end())
		return 0;
	mapper = (m->second)();

	if (submapper != -1) {
		mapper->set_submapper(submapper);
	}

	strcpy_s(mapper->filename, pathFile);
	strcpy_s(mapper->sramFilename, sramFilename);
	mapper->crc32 = crc32;
	mapper->file_length = file_length;
	reset();
	return 1;
}

int c_nes::reset()
{
	if (!cpuRam)
		cpuRam = new unsigned char[2048];
	memset(cpuRam, 0xFF, 2048);

	mapper->CloseSram();
	cpu->nes = this;
	apu2->reset();
	ppu->reset();
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
	if (mapper->LoadImage() == -1) {
		return 1;
	}
	mapper->reset();
	cpu->reset();
	joypad->reset();
	loaded = true;
	mmc3_cycles = 260;
	ppu_cycles = 341;
	return 1;
}

void c_nes::set_submapper(int submapper)
{
	mapper->set_submapper(submapper);
}

int c_nes::get_nwc_time()
{
	return mapper->get_nwc_time();
}

int c_nes::emulate_frame()
{
	if (!loaded)
		return 1;
	apu2->clear_buffer();
	for (int scanline = 0; scanline < 262; scanline++)
	{
		if (scanline == 261 || (scanline >= 0 && scanline <= 239)) {
			ppu->eval_sprites();
		}
		//ppu->run_ppu(341);
		ppu->run_ppu_line();
	}
	return 0;
}

int *c_nes::get_video()
{
	return ppu->pFrameBuffer;
}

void c_nes::set_audio_freq(double freq)
{
	apu2->set_audio_rate(freq);
}

int c_nes::get_sound_bufs(const short **buf_l, const short **buf_r)
{
	int num_samples = apu2->get_buffer(buf_l);
	*buf_r = NULL;
	return num_samples;
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