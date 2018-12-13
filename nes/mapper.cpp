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

#include "memory.h"
#include "mapper.h"
#include <fstream>

#if defined(DEBUG) | defined(_DEBUG)
	#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
	#define new DEBUG_NEW
#endif

c_mapper::c_mapper(void)
{
	ppu = 0;
	cpu = 0;
	apu2 = 0;
	writeProtectSram = false;
	sram_enabled = 1;
	hasSram = false;
	renderingBg = 0;
	sram = 0;
	mapperName = "NROM";
	submapper = 0;
	chrRom = NULL;
		four_screen = 0;
	memset(vram, 0, 4096);

	for (int i = 0; i < 4; i++)
		name_table[i] = vram;
	in_sprite_eval = 0;
	mirroring_mode = 0;
	expansion_audio = 0;
}

int c_mapper::has_expansion_audio()
{
	return expansion_audio;
}

void c_mapper::set_submapper(int submapper)
{
	this->submapper = submapper;
}

c_mapper::~c_mapper(void)
{
	CloseSram();
	if (chrRam)
		delete[] chrRom;
	if (sram)
		delete[] sram;
}

unsigned char c_mapper::ReadByte(unsigned short address)
{
	if (address >= 0x6000 && address < 0x8000)
	{
		if (sram_enabled)
			//return 0;
			return sram[address - 0x6000];
		else
			return 0;
	}
	return *(prgBank[(address >> 13) & 3] + (address & 0x1FFF));
}

void c_mapper::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x6000 && address < 0x8000)
	{
		if (sram_enabled && !writeProtectSram)
			sram[address - 0x6000] = value;
	}
}

unsigned char c_mapper::ReadChrRom(unsigned short address)
{
	return *(chrBank[(address >> 10) % 8] + (address & 0x3FF));
}

void c_mapper::WriteChrRom(unsigned short address, unsigned char value)
{
	if (chrRam)
		*(chrBank[(address >> 10) % 8] + (address & 0x3FF)) = value;
}

void c_mapper::reset(void)
{
	SetPrgBank16k(PRG_C000, header->PrgRomPageCount - 1);
	memset(vram, 0, 4096);
}

void c_mapper::SetPrgBank8k(int bank, int value)
{
	prgBank[bank] = pPrgRom + ((value % prgRomPageCount8k) * 0x2000);
}

void c_mapper::SetPrgBank16k(int bank, int value)
{
	unsigned char *base = pPrgRom + ((value % header->PrgRomPageCount) * 0x4000);
	prgBank[bank] = base;
	prgBank[bank + 1] = base + 0x2000;
}

void c_mapper::SetPrgBank32k(int value)
{
	unsigned char *base = pPrgRom + ((value % prgRomPageCount32k) * 0x8000);
	for (int i = PRG_8000; i <= PRG_E000; i++)
		prgBank[i] = base + (0x2000 * i);
}

void c_mapper::SetChrBank1k(int bank, int value)
{
	chrBank[bank] = pChrRom + ((chrRam ? (value & 0x7) : (value % chrRomPageCount1k)) * 0x400);
}

void c_mapper::SetChrBank2k(int bank, int value)
{
	unsigned char *base = pChrRom + ((chrRam ? (value & 0x3) : (value % chrRomPageCount2k)) * 0x800);
	for (int i = bank; i < bank + 2; i++)
		chrBank[i] = base + (0x400 * (i - bank));
}


void c_mapper::SetChrBank4k(int bank, int value)
{
	unsigned char *base = pChrRom + ((chrRam ? (value & 0x1) : (value % chrRomPageCount4k)) * 0x1000);
	for (int i = bank; i < bank + 4; i++)
		chrBank[i] = base + (0x400 * (i - bank));
}

void c_mapper::SetChrBank8k(int value)
{
	unsigned char *base = pChrRom + ((chrRam ? 0 : (value % header->ChrRomPageCount)) * 0x2000);
	for (int i = CHR_0000; i <= CHR_1C00; i++)
		chrBank[i] = base + (0x400 * i);
}

int c_mapper::LoadImage(void)
{
	prgRom = (prgRomBank *)(image + sizeof(iNesHeader));
	if (header->ChrRomPageCount > 0)
	{
		chrRam = false;
		chrRom = (chrRomBank *)(image + sizeof(iNesHeader) + (header->PrgRomPageCount * 16384));
	}
	else
	{
		chrRam = true;
		if (chrRom != NULL)
			delete[] chrRom;
		chrRom = new chrRomBank[1];
		memset(chrRom, 0, 8192);
	}
	pChrRom = (unsigned char *)chrRom;
	pPrgRom = (unsigned char *)prgRom;

	prgRomPageCount8k = header->PrgRomPageCount * 2;
	prgRomPageCount16k = header->PrgRomPageCount;
	prgRomPageCount32k = header->PrgRomPageCount / 2;
	if (prgRomPageCount32k < 1)
		prgRomPageCount32k = 1;
	chrRomPageCount1k = header->ChrRomPageCount * 8;
	chrRomPageCount2k = header->ChrRomPageCount * 4;
	chrRomPageCount4k = header->ChrRomPageCount * 2;

	if (header->Rcb1.Fourscreen)
		set_mirroring(MIRRORING_FOURSCREEN);
	else
		set_mirroring(header->Rcb1.Mirroring);

	for (int x = PRG_8000; x <= PRG_E000; x++)
		prgBank[x] = pPrgRom + 0x2000 * x;

	for (int x = CHR_0000; x <= CHR_1C00; x++)
		chrBank[x] = pChrRom + 0x0400 * x;

	OpenSram();

	return 0;
}

int c_mapper::OpenSram()
{
	if (!sram)
	{
		sram = new unsigned char[8192];
		memset(sram, 0, 8192);
	}

	if (header->Rcb1.Sram)
	{
		hasSram = true;
		std::ifstream file;
		file.open(sramFilename, std::ios_base::in | std::ios_base::binary);
		if (file.is_open())
		{
			file.read((char *)sram, 8192);
			file.close();
		}
	}
	return 0;

}

int c_mapper::CloseSram()
{
	if (hasSram)
	{
		std::ofstream file;
		file.open(sramFilename, std::ios_base::trunc | std::ios_base::binary);
		if (file.is_open())
		{
			file.write((char *)sram, 8192);
			file.close();
		}
	}
	if (sram)
		delete[] sram;
	sram = 0;
	return 0;
}

unsigned char c_mapper::ppu_read(unsigned short address)
{
		
	address &= 0x3FFF;

	if (address >= 0x2000)
	{
		return *(name_table[(address >> 10) & 3] + (address & 0x3FF));
	}
	else
	{
		return ReadChrRom(address);
	}
	return 0;
}

void c_mapper::ppu_write(unsigned short address, unsigned char value)
{
	address &= 0x3FFF;
	//if (address >= 0x3F00)
	//{
	//	value &= 0x3F;
	//	address &= 0x1F;
	//	image_palette[address] = value;
	//	if (!(address & 0x03))
	//	{
	//		image_palette[address ^ 0x10] = value;
	//	}
	//}
	if (address >= 0x2000)
	{
		*(name_table[(address >> 10) & 3] + (address & 0x3FF)) = value;
	}
	else
	{
		WriteChrRom(address, value);
	}
}

void c_mapper::set_mirroring(int mode)
{
	if (four_screen)
		return;
	switch (mode)
	{
	case MIRRORING_HORIZONTAL:
		name_table[0] = name_table[1] = &vram[0];
		name_table[2] = name_table[3] = &vram[0x400];
		break;
	case MIRRORING_VERTICAL:
		name_table[0] = name_table[2] = &vram[0];
		name_table[1] = name_table[3] = &vram[0x400];
		break;
	case MIRRORING_ONESCREEN_LOW:
		name_table[0] = name_table[1] =
			name_table[2] = name_table[3] = &vram[0];
		break;
	case MIRRORING_ONESCREEN_HIGH:
		name_table[0] = name_table[1] =
			name_table[2] = name_table[3] = &vram[0x400];
		break;
	case MIRRORING_FOURSCREEN:
		four_screen = true;
		name_table[0] = &vram[0];
		name_table[1] = &vram[0x400];
		name_table[2] = &vram[0x800];
		name_table[3] = &vram[0xC00];
		break;
	}
	mirroring_mode = mode;
}

int c_mapper::get_mirroring()
{
	return mirroring_mode;
}

float c_mapper::mix_audio(float sample)
{
	return sample;
}