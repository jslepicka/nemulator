#include "mapper18.h"

#include "..\cpu.h"

c_mapper18::c_mapper18()
{
	//The Lord of King
	//Magic John
	//Pizza Pop
	mapperName = "Mapper 18";
}

void c_mapper18::reset()
{
	ticks = 0;
	irq_enabled = 0;
	irq_counter = 0;
	irq_reload = 0;
	irq_size = 0;
	irq_asserted = 0;
	irq_mask = 0xFFFF;
	for (int i = 0; i < 8; i++)
		chr[i] = 0;
	for (int i = 0; i < 3; i++)
		prg[i] = 0;
	SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
}

void c_mapper18::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		switch (address & 0xF003)
		{
		case 0x8000:
			prg[0] = (prg[0] & 0xF0) | (value & 0x0F);
			break;
		case 0x8001:
			prg[0] = (prg[0] & 0x0F) | ((value & 0x0F) << 4);
			break;
		case 0x8002:
			prg[1] = (prg[1] & 0xF0) | (value & 0x0F);
			break;
		case 0x8003:
			prg[1] = (prg[1] & 0x0F) | ((value & 0x0F) << 4);
			break;
		case 0x9000:
			prg[2] = (prg[2] & 0xF0) | (value & 0x0F);
			break;
		case 0x9001:
			prg[2] = (prg[2] & 0x0F) | ((value & 0x0F) << 4);
			break;
		case 0xA000:
			chr[0] = (chr[0] & 0xF0) | (value & 0x0F);
			break;
		case 0xA001:
			chr[0] = (chr[0] & 0x0F) | ((value & 0x0F) << 4);
			break;
		case 0xA002:
			chr[1] = (chr[1] & 0xF0) | (value & 0x0F);
			break;
		case 0xA003:
			chr[1] = (chr[1] & 0x0F) | ((value & 0x0F) << 4);
			break;
		case 0xB000:
			chr[2] = (chr[2] & 0xF0) | (value & 0x0F);
			break;
		case 0xB001:
			chr[2] = (chr[2] & 0x0F) | ((value & 0x0F) << 4);
			break;
		case 0xB002:
			chr[3] = (chr[3] & 0xF0) | (value & 0x0F);
			break;
		case 0xB003:
			chr[3] = (chr[3] & 0x0F) | ((value & 0x0F) << 4);
			break;
		case 0xC000:
			chr[4] = (chr[4] & 0xF0) | (value & 0x0F);
			break;
		case 0xC001:
			chr[4] = (chr[4] & 0x0F) | ((value & 0x0F) << 4);
			break;
		case 0xC002:
			chr[5] = (chr[5] & 0xF0) | (value & 0x0F);
			break;
		case 0xC003:
			chr[5] = (chr[5] & 0x0F) | ((value & 0x0F) << 4);
			break;
		case 0xD000:
			chr[6] = (chr[6] & 0xF0) | (value & 0x0F);
			break;
		case 0xD001:
			chr[6] = (chr[6] & 0x0F) | ((value & 0x0F) << 4);
			break;
		case 0xD002:
			chr[7] = (chr[7] & 0xF0) | (value & 0x0F);
			break;
		case 0xD003:
			chr[7] = (chr[7] & 0x0F) | ((value & 0x0F) << 4);
			break;
		case 0xE000:
		case 0xE001:
		case 0xE002:
		case 0xE003:
			{
				int mask = ~(0xF << ((address & 0x3) * 4));
				irq_reload &= mask;
				irq_reload |= (value << ((address & 0x3) * 4));
			}
			break;
		case 0xF000:
			irq_counter = irq_reload;
			ack_irq();
			break;
		case 0xF001:
			irq_enabled = value & 0x1;
			irq_size = (value >> 1) & 0x7;
			if (value & 0x8)
				irq_mask = 0xF;
			else if (value & 0x4)
				irq_mask = 0xFF;
			else if (value & 0x2)
				irq_mask = 0xFFF;
			else
				irq_mask = 0xFFFF;
			ack_irq();
			break;
		case 0xF002:
			switch (value & 0x3)
			{
			case 0:
				set_mirroring(MIRRORING_HORIZONTAL);
				break;
			case 1:
				set_mirroring(MIRRORING_VERTICAL);
				break;
			case 2:
				set_mirroring(MIRRORING_ONESCREEN_LOW);
				break;
			case 3:
				set_mirroring(MIRRORING_ONESCREEN_HIGH);
				break;
			}
			break;
		}
		sync();
	}
	else
		c_mapper::WriteByte(address, value);
}

void c_mapper18::ack_irq()
{
	if (irq_asserted)
	{
		cpu->clear_irq();
		irq_asserted = 0;
	}
}

void c_mapper18::sync()
{
	for (int i = CHR_0000; i <= CHR_1C00; i++)
		SetChrBank1k(i, chr[i]);
	for (int i = PRG_8000; i <= PRG_C000; i++)
		SetPrgBank8k(i, prg[i]);
}

void c_mapper18::clock(int cycles)
{
	if (irq_enabled)
	{
		ticks += cycles;
		while (ticks > 2)
		{
			ticks -= 3;
			int temp = irq_counter & irq_mask;
			temp--;
			if ((temp & irq_mask) == irq_mask && !irq_asserted)
			{
				irq_asserted = 1;
				cpu->execute_irq();
			}

			irq_counter = (irq_counter & ~irq_mask) | (temp & irq_mask);
		}
	}
}