#include "mapper65.h"

#include "..\cpu.h"

c_mapper65::c_mapper65()
{
	//Daiku no Gen San 2
	//Kaiketsu Yanchamaru 3
	//Spartan X 2
	mapperName = "Mapper 65";
}

void c_mapper65::reset()
{
	irq_counter = 0;
	irq_reload_high = 0;
	irq_reload_low = 0;
	irq_enabled = 0;
	irq_asserted = 0;
	ticks = 0;
	SetPrgBank8k(PRG_8000, 0);
	SetPrgBank8k(PRG_A000, 1);
	SetPrgBank8k(PRG_C000, 0xFE);
	SetPrgBank8k(PRG_E000, prgRomPageCount8k-1);
}

void c_mapper65::clock(int cycles)
{
	if (irq_enabled)
	{
		ticks += cycles;
		while (ticks > 2)
		{
			ticks -= 3;
			if (irq_counter > 0)
			{
				irq_counter--;
				if (irq_counter == 0)
				{
					cpu->execute_irq();
					irq_asserted = 1;
				}
			}
		}
	}
}

void c_mapper65::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		switch (address >> 12)
		{
		case 0x8:
			SetPrgBank8k(PRG_8000, value);
			break;
		case 0x9:
			switch (address & 0x7)
			{
			case 1:
				if (value & 0x80)
					set_mirroring(MIRRORING_HORIZONTAL);
				else
					set_mirroring(MIRRORING_VERTICAL);
				break;
			case 3:
				irq_enabled = value & 0x80;
				if (irq_asserted)
				{
					cpu->clear_irq();
					irq_asserted = 0;
				}
				break;
			case 4:
				irq_counter = (irq_reload_high << 8) | irq_reload_low;
				if (irq_asserted)
				{
					cpu->clear_irq();
					irq_asserted = 0;
				}
				break;
			case 5:
				irq_reload_high = value;
				break;
			case 6:
				irq_reload_low = value;
				break;
			}
			break;
		case 0xA:
			SetPrgBank8k(PRG_A000, value);
			break;
		case 0xB:
			SetChrBank1k(address & 0x7, value);
			break;
		case 0xC:
			SetPrgBank8k(PRG_C000, value);
			break; 
		}
	}
	else
		c_mapper::WriteByte(address, value);
}