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
#include "mapper190.h"


c_mapper190::c_mapper190(void)
{
	//Magic Kid GooGoo
	mapperName = "Mapper 190";
	ram = new unsigned char[8192];
}

c_mapper190::~c_mapper190(void)
{
	delete[] ram;
}

void c_mapper190::WriteByte(unsigned short address, unsigned char value)
{
	switch ((address >> 12) & 0xE)
	{
	case 0x6:
		ram[address - 0x6000] = value;
		break;
	case 0x8:
		SetPrgBank16k(PRG_8000, value & 0x7);
		break;
	case 0xA:
		SetChrBank2k((address & 0x3) << 1, value);
		break;
	case 0xC:
		SetPrgBank16k(PRG_8000, 0x8 | (value & 0x7));
		break;
	default:
		break;
	}
}

unsigned char c_mapper190::ReadByte(unsigned short address)
{
	if (address >= 0x6000 && address < 0x8000)
	{
		return ram[address - 0x6000];
	}
	else
		return c_mapper::ReadByte(address);
}

void c_mapper190::reset()
{
	c_mapper::reset();
	set_mirroring(MIRRORING_VERTICAL);
	SetPrgBank16k(PRG_C000, 0);
	memset(ram, 0, 8192);
}