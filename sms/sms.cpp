#include "sms.h"
#include <fstream>
#include "z80.h"
#include "vdp.h"
#include "psg.h"
#include <stdio.h>
#include <Windows.h>
#include "..\crc32.h"
#include "crc.h"

#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

void strip_extension(char *path);

c_sms::c_sms(int type)
{
	this->type = type;
	z80 = new c_z80(this);
	vdp = new c_vdp(this, type);
	psg = new c_psg();
	ram = new unsigned char[8192];
	memset(cart_ram, 0, 16384);
	rom = 0;
}

c_sms::~c_sms(void)
{
	if (has_sram)
		save_sram();
	delete[] ram;
	delete psg;
	delete vdp;
	delete z80;
	if (rom)
		delete rom;
}

int c_sms::get_overscan_color()
{
	return vdp->get_overscan_color();
}

int c_sms::load()
{
	sprintf_s(pathFile, "%s\\%s", path, filename);
	std::ifstream file;
	file.open(pathFile, std::ios_base::in | std::ios_base::binary);
	if (file.fail())
		return 0;
	file.seekg(0, std::ios_base::end);
	file_length = (int)file.tellg();
	int has_header = (file_length % 1024) != 0;
	int offset = 0;
	if (has_header)
	{
		printf("has header\n");
		offset = file_length - ((file_length / 1024) * 1024);
		file_length = file_length - offset;
	}
	file.seekg(offset, std::ios_base::beg);
	if (file_length == 0 || (file_length & (file_length - 1)) || file_length < 0x2000)
	{
		return 0;
	}
	int alloc_size = file_length < 0x4000 ? 0x4000 : file_length;
	rom = new unsigned char[alloc_size];
	file.read((char *)rom, file_length);
	file.close();
	crc = get_crc32(rom, file_length);

	for (auto &c : crc_table)
	{
		if (c.crc == crc && c.has_sram)
		{
			has_sram = 1;
			load_sram();
		}
	}

	if (file_length == 0x2000)
	{
		memcpy(rom + 0x2000, rom, 0x2000);
	}
	loaded = 1;
	reset();
	return file_length;
}

void c_sms::get_sram_path(char *path)
{
	sprintf_s(path, MAX_PATH, "%s\\%s", sram_path, filename);
	strip_extension(path);
	
}

int c_sms::load_sram()
{
	char fn[MAX_PATH];
	sprintf(fn, "%s\\%s", sram_path, filename);
	strip_extension(fn);
	sprintf(sram_file_name, "%s.ram", fn);

	std::ifstream file;
	file.open(sram_file_name, std::ios_base::in | std::ios_base::binary);
	if (file.fail())
		return 1;

	file.seekg(0, std::ios_base::end);
	int l  = (int)file.tellg();

	if (l != 8192)
		return 1;

	file.seekg(0, std::ios_base::beg);

	file.read((char*)cart_ram, 8192);
	file.close();
	return 1;
}

int c_sms::save_sram()
{
	std::ofstream file;
	file.open(sram_file_name, std::ios_base::out | std::ios_base::binary);
	if (file.fail())
		return 1;
	file.write((char*)cart_ram, 8192);
	file.close();
	return 0;
}

int c_sms::reset()
{
	z80->reset();
	vdp->reset();
	psg->reset();
	memset(ram, 0x00, 8192);
	irq = 0;
	nmi = 0;
	page[0] = rom;
	page[1] = file_length > 0x4000 ? rom + 0x4000 : rom;
	page[2] = file_length > 0x8000 ? rom + 0x8000 : rom;
	nationalism = 0;
	ram_select = 0;
	joy = 0xFF;
	psg_cycles = 0;
	return 0;
}

int c_sms::emulate_frame()
{
	//for (int i = 0; i < 262; i++)
	//{
	//	for (int j = 0; j < 228; j++)
	//	{
	//		z80->execute(1);
	//		psg->clock(1);
	//	}
	//	vdp->eval_sprites();
	//	vdp->draw_scanline();
	//}

	for (int i = 0; i < 262; i++)
	{
		z80->execute(228);
		vdp->eval_sprites();
		vdp->draw_scanline();
	}
	//z80->end_frame();
	catchup_psg(228*262);
	return 0;
}

unsigned char c_sms::read_byte(unsigned short address)
{
	if (address < 0xC000)
	{
		if (address < 0x400) //always read first 1k from rom
		{
			return rom[address];
		}
		else
		{
			int p = (address >> 14) & 0x3;
			if (p == 2 && (ram_select & 0x8))
				return cart_ram[address & 0x1FFF];
			return page[p][address & 0x3FFF];
		}
	}
	else
	{
		return ram[address & 0x1FFF];
	}
}

unsigned short c_sms::read_word(unsigned short address)
{
	int lo = read_byte(address);
	int hi = read_byte(address + 1);
	return lo | (hi << 8);
}

void c_sms::write_byte(unsigned short address, unsigned char value)
{
	if (address < 0xC000)
	{
		if (address >= 0x8000 && (ram_select & 0x8))
		{
			cart_ram[address & 0x1FFF] = value;
		}
	}
	else
	{
		//ram
		if (address >= 0xFFFC)
		{
			//paging registers
			switch (address & 0x3)
			{
			case 0:
				ram_select = value;
				break;
			case 1:
			case 2:
			case 3:
			{
				int p = (address & 0x3) - 1;
				int o = (0x4000 * (value)) % file_length;
				page[p] = rom + o;
			}
			break;
			}
		}
		address &= 0x1FFF;
		ram[address] = value;
	}
}

void c_sms::write_word(unsigned short address, unsigned short value)
{
	write_byte(address, value & 0xFF);
	write_byte(address + 1, value >> 8);
}

void c_sms::catchup_psg(int end_frame)
{
	//return;
	int num_cycles = 0;
	if (end_frame)
	{
		num_cycles = end_frame - psg_cycles;
		psg_cycles = 0;
	}
	else
	{
		num_cycles = z80->pending_psg_cycles;
		psg_cycles += num_cycles;
	}
	psg->clock(num_cycles);
	z80->pending_psg_cycles = 0;
}

void c_sms::write_port(int port, unsigned char value)
{
	//printf("write %2X to port %2X\n", value, port);
	switch (port >> 6)
	{
	case 0:
		//printf("Port write to I/O or memory\n");
		if (port == 0x3F)
		{
			//printf("write %02X to nationalism\n", value);
			nationalism = value;
		}
		break;
	case 1:
		//printf("Port write to PSG\n");
		catchup_psg(0);
		psg->write(value);
		break;
	case 2:
		switch (port & 0x1)
		{
		case 0: //VDP data
			//printf("\tVDP data\n");
			vdp->write_data(value);
			break;
		case 1: //VDP control
			//printf("\tVDP control\n");
			vdp->write_control(value);
			break;
		}
		break;
	case 3:
		//printf("Port write to joypad: %02X\n", port);
		break;
	default:
		printf("Port write error\n");
		break;
	}
}

unsigned char c_sms::read_port(int port)
{
	switch (port >> 6)
	{
	case 0:
		//printf("Port read from I/O or memory %2X\n", port);
		if (type == 1)
		{
			return joy >> 31;
		}
		return 0;
	case 1:
		//printf("Port read from PSG\n");
		return vdp->get_scanline() - 1; //is this correct?  fixes earthworm jim
	case 2:
		//printf("Port read from VDP\n");
		switch (port & 0x1)
		{
		case 0: //VDP data
			//printf("Port write to VDP data\n");
			return vdp->read_data();
		case 1: //VDP control
			//printf("Port write to VDP control\n");
			return vdp->read_control();
		}
		return 0;
	case 3:
		//printf("Port read from joypad: %02X\n", port);
		if (port == 0xDC)
		{
			return joy & 0xFF;
		}
		else if (port == 0xDD)
		{
			//invert and select TH output enable bits
			int out = (nationalism ^ 0xFF) & 0xA;
			//and TH bits with TH values
			out = nationalism & (out << 4);
			//shift bits into position
			out = (out & 0x80) | ((out << 1) & 0x40);
			out = 0x3F | out;
			return out;
		}
		return 0xFF;
	default:
		printf("Port read error\n");
		return 0;
	}
}

void c_sms::set_input(int input)
{
	if (type == 0)
		nmi = input & 0x8000'0000;
	joy = ~input;
}

int *c_sms::get_video()
{
	return vdp->get_frame_buffer();
}

int c_sms::get_sound_buf(const short **buffer)
{
	return psg->get_buffer(buffer);
}

void c_sms::set_audio_freq(double freq)
{
	psg->set_audio_rate(freq);
}

void c_sms::enable_mixer()
{
	psg->enable_mixer();
}

void c_sms::disable_mixer()
{
	psg->disable_mixer();
}