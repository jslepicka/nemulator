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

#include "mapper9.h"


c_mapper9::c_mapper9(void)
{
	//Punch-Out
	mapperName = "MMC2";
}

c_mapper9::~c_mapper9(void)
{
}

void c_mapper9::reset(void)
{
	SetPrgBank8k(PRG_8000, 0);
	SetPrgBank8k(PRG_A000, prgRomPageCount8k - 3);
	SetPrgBank8k(PRG_C000, prgRomPageCount8k - 2);
	SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);

	latch0FD = 0;
	latch0FE = 4;
	latch1FD = 0;
	latch1FE = 0;
	latch0 = 0xFE;
	latch1 = 0xFE;

	SetChrBank4k(CHR_0000, latch0FE);
	SetChrBank4k(CHR_1000, latch1FE);

	memset(latch_buffer, 0, sizeof(latch_buffer));
	latch_buffer_head = 1;
	latch_buffer_tail = 0;
}

void c_mapper9::mmc2_latch(int address)
{
	switch(address &= 0x1FF0)
	{
	case 0x0FD0:
		SetChrBank4k(CHR_0000, latch0FD);
		latch0 = 0xFD;
		break;
	case 0x0FE0:
		SetChrBank4k(CHR_0000, latch0FE);
		latch0 = 0xFE;
		break;
	case 0x1FD0:
		SetChrBank4k(CHR_1000, latch1FD);
		latch1 = 0xFD;
		break;
	case 0x1FE0:
		SetChrBank4k(CHR_1000, latch1FE);
		latch1 = 0xFE;
		break;
	default:
		break;
	}
}


unsigned char c_mapper9::ReadChrRom(unsigned short address)
{
	latch_buffer[latch_buffer_head++ % 2] = address;
	unsigned char temp = c_mapper::ReadChrRom(address);
	mmc2_latch(latch_buffer[latch_buffer_tail++ % 2]);
	return temp;
}

void c_mapper9::WriteByte(unsigned short address, unsigned char value)
{
	switch (address >> 12)
	{
	case 0xA:
		SetPrgBank8k(PRG_8000, value);
		break;
	case 0xB:
		latch0FD = value & 0x1F;
		if (latch0 == 0xFD)
			SetChrBank4k(CHR_0000, value & 0x1F);
		break;
	case 0xC:
		latch0FE = value & 0x1F;
		if (latch0 == 0xFE)
			SetChrBank4k(CHR_0000, value & 0x1F);
		break;
	case 0xD:
		latch1FD = value & 0x1F;
		if (latch1 == 0xFD)
			SetChrBank4k(CHR_1000, value & 0x1F);
		break;
	case 0xE:
		latch1FE = value & 0x1F;
		if (latch1 == 0xFE)
			SetChrBank4k(CHR_1000, value & 0x1F);
		break;
	case 0xF:
		if (value & 0x1)
			set_mirroring(MIRRORING_HORIZONTAL);
		else
			set_mirroring(MIRRORING_VERTICAL);
		break;
	default:
		c_mapper::WriteByte(address, value);
		break;
	}
}