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
#include "mapper4.h"

#include "..\cpu.h"
#include "..\ppu.h"

c_mapper4::c_mapper4(void)
{
	mapperName = "MMC3";
	four_screen = 0;
}

c_mapper4::~c_mapper4(void)
{
}

void c_mapper4::Sync(void)
{
	SetPrgBank8k(PRG_8000 ^ prg_xor, (prg[0] & prg_mask) + prg_offset);
	SetPrgBank8k(PRG_A000, (prg[1] & prg_mask) + prg_offset);
	SetPrgBank8k(PRG_C000 ^ prg_xor, ((last_prg_page - 1) & prg_mask) + prg_offset);
	SetPrgBank8k(PRG_E000, (last_prg_page & prg_mask) + prg_offset);

	SetChrBank1k(CHR_0000 ^ chr_xor, (chr[0] & chr_mask) + chr_offset);
	SetChrBank1k(CHR_0400 ^ chr_xor, (chr[0] & chr_mask) + 1 + chr_offset);
	SetChrBank1k(CHR_0800 ^ chr_xor, (chr[1] & chr_mask) + chr_offset);
	SetChrBank1k(CHR_0C00 ^ chr_xor, (chr[1] & chr_mask) + 1 + chr_offset);
	SetChrBank1k(CHR_1000 ^ chr_xor, (chr[2] & chr_mask) + chr_offset);
	SetChrBank1k(CHR_1400 ^ chr_xor, (chr[3] & chr_mask) + chr_offset);
	SetChrBank1k(CHR_1800 ^ chr_xor, (chr[4] & chr_mask) + chr_offset);
	SetChrBank1k(CHR_1C00 ^ chr_xor, (chr[5] & chr_mask) + chr_offset);
}

unsigned char c_mapper4::ReadByte(unsigned short address)
{
	if (submapper == 1)
	{
		switch (address >> 12)
		{
		case 6:
		case 7:
			return address >> 8;
			break;
		default:
			return c_mapper::ReadByte(address);
		}
	}
	else
		return c_mapper::ReadByte(address);
}

void c_mapper4::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		switch (address & 0xE001)
		{
		case 0x8000:
			commandNumber = value & 0x07;
			prgSelect = value & 0x40 ? true : false;
			prg_xor = prgSelect << 1;
			chrXor = value & 0x80 ? true : false;
			chr_xor = chrXor << 2;
			Sync();
			break;
		case 0x8001:
			switch (commandNumber)
			{
			case 0:
			case 1:
				chr[commandNumber] = value & 0xFE;
				break;
			case 2:
			case 3:
			case 4:
			case 5:
				chr[commandNumber] = value;
				break;
			case 6:
			case 7:
				prg[commandNumber - 6] = value;
				break;
			}
			Sync();
			break;
		case 0xa000:
			if (!four_screen) {
				if (value & 0x01)
					set_mirroring(MIRRORING_HORIZONTAL);
				else
					set_mirroring(MIRRORING_VERTICAL);
			}
			break;
		case 0xa001:
			writeProtectSram = value & 0x40 ? true : false;
			sram_enabled = value & 0x80;
			break;
		case 0xc000:
			//if (value == 0)
			//	fireIRQ = true;
			irqCounterReload = value;
			//irqCounter = Value;
			break;
		case 0xc001:
			irqReload = true;
			//probably should be commented out
			//irqCounter = 0;
			//irqLatch = Value;
			break;
		case 0xe000:
			irqEnabled = false;
			if (irq_asserted)
			{
				cpu->clear_irq();
				irq_asserted = 0;
			}
			break;
		case 0xe001:
			irqEnabled = true;
			break;
		default:
			break;
		}
	}
	else
		c_mapper::WriteByte(address, value);
}


unsigned char c_mapper4::ppu_read(unsigned short address)
{
	if (nes->emulation_mode == c_nes::modes::EMULATION_MODE_ACCURATE)
	{
		if (!in_sprite_eval)
			check_a12(address);
	}
	return c_mapper::ppu_read(address);
}

void c_mapper4::ppu_write(unsigned short address, unsigned char value)
{
	if (nes->emulation_mode == c_nes::modes::EMULATION_MODE_ACCURATE)
	{
		if (!in_sprite_eval)
			check_a12(address);
	}
	c_mapper::ppu_write(address, value);
}

void c_mapper4::clock(int cycles)
{
	if (!(current_address & 0x1000))
	{
		low_count -= cycles;
		if (low_count < 0)
			low_count = 0;
	}
	else
		low_count = 8;
}

void c_mapper4::check_a12(int address)
{
	current_address = address;

	if ((address & 0x1000) && low_count == 0)
		clock_irq_counter();
}


void c_mapper4::clock_irq_counter()
{
	/*

	MMC3A and non-Sharp MMC3B edge-detect
	Sharp-MMC3B and MMC3C level-detect *DEFAULT BEHAVIOR*

	Only matters when irqCounterReload is set to 0, but some games rely on specific behavior.
	These games need to be CRC-detected.

	*/
	unsigned char previous_count = irqCounter;
	if (irqCounter == 0 || irqReload)
	{
		irqCounter = irqCounterReload;
		irqReload = false;
	}
	else
	{
		irqCounter--;
	}

	if ((irq_mode == 1 || previous_count != 0) && irqCounter == 0 && irqEnabled)
	{
		if (!irq_asserted)
		{
			fire_irq();
		}
	}
}

void c_mapper4::fire_irq()
{
	cpu->execute_irq();
	irq_asserted = 1;
}

void c_mapper4::mmc3_check_a12()
{
	if (ppu->DoA12())
		clock_irq_counter();
}

void c_mapper4::reset(void)
{
	if (nes->header->Rcb1.Fourscreen) {
		four_screen = 1;
	}
	if (crc32 == 0x404B2E8B) { //Rad Racer 2
		set_mirroring(MIRRORING_FOURSCREEN);
		four_screen = 1;
	}

	//SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);

	irqCounter = 0;
	irqEnabled = false;
	irqLatch = 0;
	irqReload = false;
	irqCounterReload = 0;
	fireIRQ = false;
	previous_address = 0;
	current_address = 100;
	low_count = 0;
	last_prg_page = prgRomPageCount8k - 1;
	irq_mode = 1;
	irq_asserted = 0;

	prg_xor = 0;
	chr_xor = 0;
	prg_offset = 0;
	chr_offset = 0;
	prg_mask = 0xFF;
	chr_mask = 0xFF;

	if (crc32 == 0x97B6CB19) //Aladdin (SuperGame) (Mapper 4)
		irq_mode = 0;

	//TODO: Not sure if these are the correct initialization values

	prg[0] = 0;
	prg[1] = 1;
	chr[0] = 0;
	chr[1] = 2;
	chr[2] = 4;
	chr[3] = 5;
	chr[4] = 6;
	chr[5] = 7;
	Sync();
}
