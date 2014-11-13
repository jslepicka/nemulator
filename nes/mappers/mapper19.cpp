#include "mapper19.h"

#include "..\cpu.h"

c_mapper19::c_mapper19()
{
	chr_ram = new unsigned char[1024*32];
	mapperName = "Mapper 19";
}

c_mapper19::~c_mapper19()
{
	delete[] chr_ram;
}

void c_mapper19::reset()
{
	memset(chr_ram, 0, 1024*32);
	irq_enabled = 0;
	irq_counter = 0;
	reg_e800 = 0;
	irq_asserted = 0;
	ticks = 0;
	memset(chr_regs, 0, sizeof(chr_regs));
	memset(mirroring_regs, 0, sizeof(mirroring_regs));
	SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
	if (crc32 == 0xC247CC80) //Family Circuit '91
	{
		submapper = 1;
		mapperName = "Mapper 19 (1)";
	}
}

void c_mapper19::WriteByte(unsigned short address, unsigned char value)
{
	if ((address >= 0x4800 && address < 0x6000) || (address >= 0x8000))
	{
		switch (address & 0xF800)
		{
		case 0x4800:
			//sound
			break;
		case 0x5000:
			irq_counter = (irq_counter & 0xFF00) | (value & 0xFF);
			//is this wrong?  docs don't say anything about setting this, but final lap is otherwise broken.
			//irq_counter |= 0x80;
			if (irq_asserted)
			{
				cpu->clear_irq();
				irq_asserted = 0;
			}
			break;
		case 0x5800:
			irq_counter = (irq_counter & 0x00FF) | ((value & 0x7F) << 8);
			irq_enabled = value & 0x80;
			if (irq_asserted)
			{
				cpu->clear_irq();
				irq_asserted = 0;
			}
			break;
		case 0x8000: chr_regs[0] = value; sync_chr(); break;
		case 0x8800: chr_regs[1] = value; sync_chr(); break;
		case 0x9000: chr_regs[2] = value; sync_chr(); break;
		case 0x9800: chr_regs[3] = value; sync_chr(); break;
		case 0xA000: chr_regs[4] = value; sync_chr(); break;
		case 0xA800: chr_regs[5] = value; sync_chr(); break;
		case 0xB000: chr_regs[6] = value; sync_chr(); break;
		case 0xB800: chr_regs[7] = value; sync_chr(); break;
		case 0xC000: mirroring_regs[0] = value; sync_mirroring(); break;
		case 0xC800: mirroring_regs[1] = value; sync_mirroring(); break;
		case 0xD000: mirroring_regs[2] = value; sync_mirroring(); break;
		case 0xD800: mirroring_regs[3] = value; sync_mirroring(); break;
		case 0xE000:
			SetPrgBank8k(PRG_8000, value & 0x3F);
			break;
		case 0xE800:
			reg_e800 = value;
			SetPrgBank8k(PRG_A000, value & 0x3F);
			sync_chr();
			break;
		case 0xF000:
			SetPrgBank8k(PRG_C000, value & 0x3F);
			break;
		case 0xF800:
			//sound
			break;
		}
	}
	else
		c_mapper::WriteByte(address, value);
}

unsigned char c_mapper19::ReadByte(unsigned short address)
{
	if (address >= 0x4800 && address < 0x6000)
	{
		switch (address & 0xF800)
		{
		case 0x5000:
			if (irq_asserted)
			{
				cpu->clear_irq();
				irq_asserted = 0;
			}
			return irq_counter & 0xFF;
		case 0x5800:
			if (irq_asserted)
			{
				cpu->clear_irq();
				irq_asserted = 0;
			}
			return (irq_counter & 0x7F00) >> 8;
		}
		return 0;
	}
	else
		return c_mapper::ReadByte(address);
}

void c_mapper19::sync_chr()
{
	for (int i = 0; i < 8; i++)
	{
		if (chr_regs[i] < 0xE0 || (i < 4 && (reg_e800 & 0x40)) || (i > 3 && (reg_e800 & 0x80)))
			SetChrBank1k(i, chr_regs[i]);
		else
		{
			chrBank[i] = chr_ram + (chr_regs[i] - 0xE0) * 0x400;
		}
	}
}

void c_mapper19::sync_mirroring()
{
	if (submapper == 1)
		return;
	for (int i = 0; i < 4; i++)
	{
		if (mirroring_regs[i] >= 0xE0)
			name_table[i] = &vram[0x400 * (mirroring_regs[i] & 0x1)];
		else
			name_table[i] = pChrRom + 0x400 * (mirroring_regs[i] % chrRomPageCount1k);
	}
}

void c_mapper19::clock(int cycles)
{
	ticks += cycles;
	while (ticks > 2)
	{
		//if (irq_counter & 0x8000)
		//{
		//	irq_counter++;
		//	if (!(irq_counter & 0x8000) && !irq_asserted)
		//	{
		//		cpu->execute_irq();
		//		irq_asserted = 1;
		//	}
		//}
		if (irq_enabled)
		{
			if (irq_counter == 0x7FFF)
			{
				if (irq_enabled && !irq_asserted)
				{
					//irq_enabled = 0;
					cpu->execute_irq();
					irq_asserted = 1;
				}
			}
			else
				irq_counter++;
		}

		ticks -= 3;
	}
}