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

#include "mapper1.h"

//#include "ppu.h"

c_mapper1::c_mapper1(void)
{
	mapperName = "MMC1";
}

c_mapper1::~c_mapper1(void)
{
}

void c_mapper1::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000 && !ignore_writes)
	{
		ignore_writes = 1;
		if (value & 0x80)
		{
			val = 0;
			count = 0;
			regs[0] |= 0x0C;
			//return;
		}
		else
		{

			val |= ((value & 0x01) << count++);
			if (count == 5)
			{
				regs[(address >> 13) - 4] = val;
				count = 0;
				val = 0;
				Sync();
			}
		}
	}
	else
		c_mapper::WriteByte(address, value);

	return;
}

void c_mapper1::Sync()
{
	sync_prg();
	sync_chr();
	sync_mirror();
}

void c_mapper1::sync_prg()
{
	//reg[0] chr mode, prg size, slot select, mirroring

	//reg[1] chr reg 0

	//reg[2] chr reg 1

	//reg[3] prg reg, wram

	if (regs[0] & 0x08) //16k mode
	{
		if (regs[0] & 0x04) //$8000 swappable, $C000 fixed to page $0F
		{
			if (submapper == 1)
			{
				SetPrgBank16k(PRG_8000, (regs[3] & 0x0F) | (regs[1] & 0x10));
				SetPrgBank16k(PRG_C000, 0x0F | (regs[1] & 0x10));
			}
			else
			{
				SetPrgBank16k(PRG_8000, regs[3] & 0x0F);
				SetPrgBank16k(PRG_C000, 0x0F);
			}

			//SUROM
			//SetPrgBank16k(PRG_8000, (regs[3] & 0x0F) | (regs[1] & 0x10));
			//SetPrgBank16k(PRG_C000, 0x0F | (regs[1] & 0x10));

		}
		else //$C000 swappable, $8000 fixed to page $00
		{
			if (submapper == 1)
			{
				SetPrgBank16k(PRG_C000, (regs[3] & 0x0F) | (regs[1] & 0x10));
				SetPrgBank16k(PRG_8000, 0 | (regs[1] & 0x10));
			}
			else
			{
				SetPrgBank16k(PRG_C000, regs[3] & 0x0F);
				SetPrgBank16k(PRG_8000, 0);
			}

			//SUROM
			//SetPrgBank16k(PRG_C000, (regs[3] & 0x0F) | (regs[1] & 0x10));
			//SetPrgBank16k(PRG_8000, 0 | (regs[1] & 0x10));
		}
	}
	else //32k mode
	{
		SetPrgBank32k((regs[3] & 0x0F) >> 1);
	}
}

void c_mapper1::sync_chr()
{
	if (regs[0] & 0x10) //4k chr mode
	{
		SetChrBank4k(CHR_0000, regs[1] & 0x1F);
		SetChrBank4k(CHR_1000, regs[2] & 0x1F);
	}
	else //8k chr mode
	{
		SetChrBank8k((regs[1] & 0x1F) >> 1);
		//SetChrBank4k(CHR_0000, regs[1] & 0x1E);
		//SetChrBank4k(CHR_1000, (regs[1] & 0x1E) | 1);
	}
}

void c_mapper1::sync_mirror()
{
	switch (regs[0] & 0x03)
	{
	case 0x00:
		set_mirroring(MIRRORING_ONESCREEN_LOW);
		break;
	case 0x01:
		set_mirroring(MIRRORING_ONESCREEN_HIGH);
		break;
	case 0x02:
		set_mirroring(MIRRORING_VERTICAL);
		break;
	case 0x03:
		set_mirroring(MIRRORING_HORIZONTAL);
		break;
	}
}

void c_mapper1::clock(int cycles)
{
	ignore_writes = 0;
}

void c_mapper1::reset(void)
{
	if (
		crc32 == 0xA49B48B8 ||	//Dragon Quest III - Soshite Densetsu e... (J) (PRG0) [!].nes
		crc32 == 0x9C654F15 ||	//Dragon Quest III - Soshite Densetsu e... (J) (PRG0) [o1].nes
		crc32 == 0x9F830358 ||	//Dragon Quest III - Soshite Densetsu e... (J) (PRG0) [T+Eng0.0111_Spinner_8].nes
		crc32 == 0x869501CA ||	//Dragon Quest III - Soshite Densetsu e... (J) (PRG1) [!].nes
		crc32 == 0x0794F2A5 ||	//Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [!].nes
		crc32 == 0xAC413EB0 ||	//Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [b1].nes
		crc32 == 0x579EEE6B ||	//Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [b2].nes
		crc32 == 0x545D027B ||	//Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [b3].nes
		crc32 == 0x3EBCE9D3 ||	//Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [b4].nes
		crc32 == 0xB4CAFFFB ||	//Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [o1].nes
		crc32 == 0x1613A3E8 ||	//Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [o2].nes
		crc32 == 0xB7BDE3FC ||	//Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [o3].nes
		crc32 == 0x9CAD70D7 ||	//Dragon Quest IV Mayuge V1.0 (DQ4 Hack).nes
		crc32 == 0xA86A5318 ||	//Dragon Warrior III (U) [!].nes
		crc32 == 0x759EE933 ||	//Dragon Warrior III (U) [b1].nes
		crc32 == 0x1ED7F280 ||	//Dragon Warrior III (U) [b1][o1].nes
		crc32 == 0xA48352DD ||	//Dragon Warrior III (U) [b2].nes
		crc32 == 0xE1D847F8 ||	//Dragon Warrior III (U) [b2][o1].nes
		crc32 == 0xE02BD69D ||	//Dragon Warrior III (U) [b3].nes
		crc32 == 0x74597343 ||	//Dragon Warrior III (U) [b4].nes
		crc32 == 0x26733857 ||	//Dragon Warrior III (U) [b5].nes
		crc32 == 0xA6BE0EA7 ||	//Dragon Warrior III (U) [b6].nes
		crc32 == 0x29BD4B11 ||	//Dragon Warrior III (U) [o1].nes
		crc32 == 0x9898CA71 ||	//Dragon Warrior III (U) [o2].nes
		crc32 == 0xA44021DB ||	//Dragon Warrior III (U) [T+Fre1.0_Generation IX].nes
		crc32 == 0x9549652C ||	//Dragon Warrior III (U) [T+Por1.1_CBT].nes 9549652C
		crc32 == 0xAABF628C ||	//Dragon Warrior III (U) [T-FreBeta_Generation IX].nes
		crc32 == 0x9B9D3893 ||	//Dragon Warrior III (U) [T-Por_CBT].nes
		crc32 == 0x39EEEE89 ||	//Dragon Warrior III Special Ed. V0.5 (Hack).nes
		crc32 == 0x506E259D ||	//Dragon Warrior IV (U) [!].nes
		crc32 == 0x41413B06 ||	//Dragon Warrior IV (U) [b1].nes
		crc32 == 0x2E91EB15 ||	//Dragon Warrior IV (U) [o1].nes
		crc32 == 0x06390812 ||	//Dragon Warrior IV (U) [o2].nes
		crc32 == 0xFC2B6281 ||	//Dragon Warrior IV (U) [o3].nes
		crc32 == 0x030AB0B2 ||	//Dragon Warrior IV (U) [o4].nes
		crc32 == 0xAB43AA55 ||	//Dragon Warrior IV (U) [o5].nes
		crc32 == 0x97F8C475 ||	//Dragon Warrior IV (U) [o6].nes
		crc32 == 0x44544B8A ||	//Final Fantasy 99 (DQ3 Hack) [a1].nes
		crc32 == 0xB8F8D911 ||	//Final Fantasy 99 (DQ3 Hack) [a1][o1].nes
		crc32 == 0x70FA1880	||	//Final Fantasy 99 (DQ3 Hack).nes
		crc32 == 0xCEE5857B ||  //Ninjara Hoi! (J) [!].nes
		crc32 == 0xCB8F9273 ||  //Ninjara Hoi! (J) [b1].nes
		crc32 == 0xFFBC61C5 ||  //Ninjara Hoi! (J) [b2].nes
		crc32 == 0xB061F6E2     //Ninjara Hoi! (J) [o1].nes
		)
	{
		submapper = 1;
	}
	regs[0] = 0x0C;
	regs[1] = 0;
	regs[2] = 0;
	regs[3] = 0;
	val = 0;
	count = 0;
	ignore_writes = 0;
	cycle_count = 1;
	Sync();

}