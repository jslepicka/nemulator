#include "sms.h"
#include <fstream>
#include "z80.h"
#include "vdp.h"
#include "psg.h"
#include <stdio.h>
#include <Windows.h>

c_sms::c_sms(void)
{
	z80 = new c_z80(this);
	vdp = new c_vdp(this);
	psg = new c_psg();
	ram = new unsigned char[8192];
}


c_sms::~c_sms(void)
{
	delete[] ram;
	delete psg;
	delete vdp;
	delete z80;
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
	rom = new unsigned char[file_length];
	file.read((char *)rom, file_length);
	file.close();
	loaded = 1;
	reset();
	return file_length;
}

int c_sms::reset()
{
	z80->reset();
	vdp->reset();
	psg->reset();
	memset(ram, 0x00, 8192);
	irq = 0;
	page[0] = rom;
	page[1] = rom + (0x4000 % file_length);
	page[2] = rom + (0x8000 % file_length);
	nationalism = 0;
	ram_select = 0;
	return 0;
}

int c_sms::emulate_frame()
{
	for (int i = 0; i < 262; i++)
	{
		for (int j = 0; j < 228; j++)
		{
			z80->execute(1);
			psg->clock(1);
		}
		//cpu->execute(228);
		//psg->clock(228);
		vdp->eval_sprites();
		vdp->draw_scanline();
	}
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
			nationalism = value & 0x80;
			nationalism |= ((value & 0x20) << 1);
		}
		break;
	case 1:
		//printf("Port write to PSG\n");
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
		return 0;
	case 1:
		//printf("Port read from PSG\n");
		return vdp->get_scanline();
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
			int ret = 0xFF;
			if (GetAsyncKeyState(0x5A) & 0x8000)
			{
				ret &= (~0x10);
			}
			if (GetAsyncKeyState(0x58) & 0x8000)
			{
				ret &= (~0x20);
			}
			if (GetAsyncKeyState(VK_UP) & 0x8000)
			{
				ret &= (~0x01);
			}
			if (GetAsyncKeyState(VK_DOWN) & 0x8000)
			{
				ret &= (~0x02);
			}
			if (GetAsyncKeyState(VK_LEFT) & 0x8000)
			{
				ret &= (~0x04);
			}
			if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
			{
				ret &= (~0x08);
			}
			return ret;
		}
		else if (port == 0xDD)
		{
			int val = 0x3F | nationalism;
			return val;
		}
		return 0xFF;
	default:
		printf("Port read error\n");
		return 0;
	}
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