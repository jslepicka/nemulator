#include "mapper16.h"

#include "..\cpu.h"

c_mapper16::c_mapper16()
{
	//Dragon Ball Z, etc.
	//No EPROM support
	mapperName = "Mapper 16";
}

c_mapper16::~c_mapper16()
{
}

void c_mapper16::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x6000)
	{
		switch (address & 0xF)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			SetChrBank1k(address & 0xF, value);
			break;
		case 8:
			SetPrgBank16k(PRG_8000, value);
			break;
		case 9:
			switch (value & 0x3)
			{
			case 0:
				set_mirroring(MIRRORING_VERTICAL);
				break;
			case 1:
				set_mirroring(MIRRORING_HORIZONTAL);
				break;
			case 2:
				set_mirroring(MIRRORING_ONESCREEN_LOW);
				break;
			case 3:
				set_mirroring(MIRRORING_ONESCREEN_HIGH);
				break;
			}
			break;
		case 0xA:
			irq_enabled = value & 0x1;
			if (irq_asserted)
			{
				cpu->clear_irq();
				irq_asserted = 0;
			}
			break;
		case 0xB:
			irq_counter = (irq_counter & 0xFF00) | value;
			break;
		case 0xC:
			irq_counter = (irq_counter & 0xFF) | (value << 8);
			break;
		case 0xD:
			break;
		default:
			break;
		}
	}
	else
		c_mapper::WriteByte(address, value);
}

void c_mapper16::reset()
{
	SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
	irq_counter = 0;
	irq_enabled = 0;
	irq_asserted = 0;
	ticks = 0;
}

void c_mapper16::clock(int cycles)
{
	ticks += cycles;
	while (ticks > 2)
	{
		ticks -= 3;
		if (irq_enabled)
		{
			int prev_counter = irq_counter;
			irq_counter--;

			if (prev_counter == 1 && irq_counter == 0)
			{
				if (!irq_asserted)
				{
					cpu->execute_irq();
					irq_asserted = 1;
				}
			}
		}
	}
}