#include "mapper5.h"

#include "..\cpu.h"
#include "..\ppu.h"

c_mapper5::c_mapper5()
{
	mapperName = "MMC5";
	prg_ram = new unsigned char[65535];
	exram = new unsigned char[1024];
	memset(exram, 0, 1024);
	memset(prg_ram, 0, 65535);
}

c_mapper5::~c_mapper5()
{
	delete[] exram;
	delete[] prg_ram;
}

void c_mapper5::mmc5_ppu_write(unsigned short address, unsigned char value)
{
	if (address == 0x2001)
	{
		drawing_enabled = (value & 0x18);
		if (inFrame && !drawing_enabled)
			inFrame = 0;
	}
}

void c_mapper5::WriteByte(unsigned short address, unsigned char value)
{
	switch (address >> 12)
	{
	case 5:
		if (address >= 0x5C00 && address < 0x6000)
		{
			if (exram_mode == 2 || (exram_mode < 2 && inFrame))
			{
				exram[address - 0x5C00] = value;
			}
			else if (exram_mode < 2)
			{
				exram[address - 0x5C00] = 0;
			}
			return;
		}
		switch (address)
		{
		case 0x5000:
		case 0x5001:
		case 0x5002:
		case 0x5003:
		case 0x5004:
		case 0x5005:
		case 0x5006:
		case 0x5007:
		case 0x5010:
		case 0x5011:
		case 0x5015:
			//sound
			break;
		case 0x5100:
			prgMode = value & 0x03;
			Sync();
			break;
		case 0x5101:
			chrMode = value & 0x03;
			Sync();
			break;
		case 0x5102:
		case 0x5103:
			prg_ram_protect[address - 0x5102] = value & 0x3;
			break;
		case 0x5104:
			exram_mode = value & 0x3;
			if (exram_mode == 1)
			{
				int a = 1;
			}
			break;
		case 0x5105:
			//TODO: 0x01 mask is definitely wrong, but there's no ExRAM support yet, so...
			//ppu->SetNameTable(0, 0x2000 + 0x400 * (value & 0x01));
			//ppu->SetNameTable(1, 0x2000 + 0x400 * ((value >> 2) & 0x01));
			//ppu->SetNameTable(2, 0x2000 + 0x400 * ((value >> 4) & 0x01));
			//ppu->SetNameTable(3, 0x2000 + 0x400 * ((value >> 6) & 0x01));
			//name_table[0] = &vram[0x400 * (value & 0x01)];
			//name_table[1] = &vram[0x400 * ((value >> 2) & 0x01)];
			//name_table[2] = &vram[0x400 * ((value >> 4) & 0x01)];
			//name_table[3] = &vram[0x400 * ((value >> 6) & 0x01)];
			for (int i = 0; i < 4; i++)
			{
				switch ((value >> (i*2)) & 0x3)
				{
				case 0:
					name_table[i] = vram;
					break;
				case 1:
					name_table[i] = vram + 0x400;
					break;
				case 2:
					name_table[i] = exram;
					break;
				case 3:
					name_table[i] = 0;
					break;
				}
			}
			//Sync();
			break;
		case 0x5106:
			fillTile = value;
			break;
		case 0x5107:
			fillColor = value & 0x03;
			fillColor |= fillColor << 2;
			fillColor |= fillColor << 4;
			break;
		case 0x5113:
			prg_6000 = prg_ram + (value & 0x7) * 0x2000;
			break;
		case 0x5114:
		case 0x5115:
		case 0x5116:
		case 0x5117:
			prg_reg[address - 0x5114] = value;
			Sync();
			break;
		case 0x5120:
		case 0x5121:
		case 0x5122:
		case 0x5123:
		case 0x5124:
		case 0x5125:
		case 0x5126:
		case 0x5127:
			banks[address-0x5120] = value | (chr_high << 8);
			lastBank = 0;
			Sync();
			break;
		case 0x5128:
		case 0x5129:
		case 0x512A:
		case 0x512B:
			banks[(address-0x5128)+8] = value | (chr_high << 8);
			lastBank = 1;
			Sync();
			break;
		case 0x5130:
			chr_high = value & 0x3;
			break;
		case 0x5200:
			split_control = value;
			break;
		case 0x5201:
			split_scroll = value;
			break;
		case 0x5202:
			split_page = value;
			break;
		case 0x5203:
			irqTarget = value;
			//cpu->clear_irq();
			break;
		case 0x5204:
			irqEnable = value & 0x80;
			if (irqEnable && irqPending && !irq_asserted)
			{
				cpu->execute_irq();
				irq_asserted = 1;
			}
			break;
		case 0x5205:
			multiplicand = value;
			break;
		case 0x5206:
			multiplier = value;
			break;
		default:
			int a = 1;
			break;

		}
		break;
	case 6:
	case 7:
		if (prg_ram_protect[0] == 0x2 && prg_ram_protect[1] == 0x1)
			*(prg_6000 + (address & 0x1FFF)) = value;
		break;
	}

}

void c_mapper5::SetPrgBank8k(int bank, int value)
{
	prgBank[bank] = (value & 0x80 ? pPrgRom : prg_ram) + (((value & 0x7F) % prgRomPageCount8k) * 0x2000);
}

void c_mapper5::SetPrgBank16k(int bank, int value)
{
	unsigned char *base = (value & 0x80 ? pPrgRom : prg_ram) + ((((value & 0x7F) >> 1) % header->PrgRomPageCount) * 0x4000);
	prgBank[bank] = base;
	prgBank[bank + 1] = base + 0x2000;
}

void c_mapper5::SetPrgBank32k(int value)
{
	unsigned char *base = (value & 0x80 ? pPrgRom : prg_ram) + ((((value & 0x7F) >> 2) % prgRomPageCount32k) * 0x8000);
	for (int i = PRG_8000; i <= PRG_E000; i++)
		prgBank[i] = base + (0x2000 * i);
}

void c_mapper5::SetChrBank1k(unsigned char *b[8], int bank, int value)
{
	b[bank] = pChrRom + ((chrRam ? value : (value % chrRomPageCount1k)) * 0x400);
}

void c_mapper5::SetChrBank2k(unsigned char *b[8], int bank, int value)
{
	unsigned char *base = pChrRom + ((chrRam ? value : (value % chrRomPageCount2k)) * 0x800);
	for (int i = bank; i < bank + 2; i++)
		b[i] = base + (0x400 * (i - bank));
}


void c_mapper5::SetChrBank4k(unsigned char *b[8], int bank, int value)
{
	unsigned char *base = pChrRom + ((chrRam ? value : (value % chrRomPageCount4k)) * 0x1000);
	for (int i = bank; i < bank + 4; i++)
		b[i] = base + (0x400 * (i - bank));
}

void c_mapper5::SetChrBank8k(unsigned char *b[8], int value)
{
	unsigned char *base = pChrRom + ((chrRam ? value : (value % header->ChrRomPageCount)) * 0x2000);
	for (int i = CHR_0000; i <= CHR_1C00; i++)
		b[i] = base + (0x400 * i);
}

int c_mapper5::in_split_region()
{
	int tile = cycle >= 320 ? (cycle - 320) / 8 : (cycle + 16) / 8;
	if (tile > 33)
		return 0;
	if ((tile < 34) &&
		(((split_control & 0x80) && (split_control & 0x40) && tile >= (split_control & 0x1F)) ||
		((split_control & 0x80) && !(split_control & 0x40) && tile < (split_control & 0x1F))))
	{
		return tile;
	}
	else
		return 0;


}
unsigned char c_mapper5::ppu_read(unsigned short address)
{
	address &= 0x3FFF;

	if ((address & 0x23FF) >= 0x23C0 && exram_mode == 1 && ppu->drawingBg)
	{
		int attr = (exram[last_tile] >> 6) & 0x03;
		attr |= attr << 2;
		attr |= attr << 4;
		return attr;
	}
	else if ((address & 0x23FF) >= 0x23C0 && using_fill_table)
	{
		using_fill_table = 0;
		return fillColor;
	}
	else if ((address & 0x23FF) >= 0x23C0 && in_split_region())
	{
		//need to fix split table attributes
		return 0xAA;
	}
	else if (address >= 0x2000)
	{
		last_tile = address & 0x3FF;

		if (int tile = in_split_region())
		{
			int t = vscroll;
			t = (t & 0xF8) << 2;
			t |= (tile & 0x1F);
			return *(exram + (t & 0x3FF));
		}
		else
			if (name_table[address >> 10 & 3] == 0)
			{
				using_fill_table = 1;
				return fillTile;
			}
			else
				return *(name_table[(address >> 10) & 3] + (address & 0x3FF));
	}
	else
	{
		return ReadChrRom(address);
	}
	return 0;
}

void c_mapper5::ppu_write(unsigned short address, unsigned char value)
{
	address &= 0x3FFF;

	if (address >= 0x2000 && name_table[(address >> 10) & 3] == 0)
		return;
	else
		c_mapper::ppu_write(address, value);
}

unsigned char c_mapper5::ReadChrRom(unsigned short address)
{
	int page = (exram[last_tile] & 0x3F) | (chr_high << 6);
	unsigned char *base = pChrRom + ((page % chrRomPageCount4k) * 0x1000);
	unsigned char *split_base = pChrRom + ((split_page % chrRomPageCount4k) * 0x1000);
	unsigned char *bank8 = lastBank ? bankB[address >> 10] : bankA[address >> 10];

	int scroll_addr = (vscroll & 0x7) | (address & ~0x7);

	if (!inFrame)
	{
		return *(bank8 + (address & 0x3FF));
	}

	if (ppu->GetSpriteSize())
	{
		if (ppu->drawingBg)
		{
			if (exram_mode == 1)
			{
				return *(base + (address & 0xFFF));
			}
			else if (in_split_region())
			{
				return *(split_base + (scroll_addr & 0xFFF));
			}
			else
				return *(bankB[address >> 10] + (address & 0x3FF));
		}
		else
			return *(bankA[address >> 10] + (address & 0x3FF));
	}
	else
	{
		if (ppu->drawingBg)
		{
			if (exram_mode == 1)
			{
				return *(base + (address & 0xFFF));
			}
			else if (in_split_region())
			{
				return *(split_base + (scroll_addr & 0xFFF));
			}
			else
				return *(bank8 + (address & 0x3FF));
		}
		else
			return *(bank8 + (address & 0x3FF));

		/*if (lastBank)
		return *(bankB[address >> 10] + (address & 0x3FF));
		else
		return *(bankA[address >> 10] + (address & 0x3FF));*/
	}
}

void c_mapper5::Sync()
{
	switch(chrMode)
	{
	case 0:
		SetChrBank8k(bankA, banks[7]);
		SetChrBank8k(bankB, banks[3+8]);
		break;
	case 1:
		SetChrBank4k(bankA, CHR_0000, banks[3]);
		SetChrBank4k(bankA, CHR_1000, banks[7]);
		SetChrBank4k(bankB, CHR_0000, banks[3+8]);
		SetChrBank4k(bankB, CHR_1000, banks[3+8]);
		break;
	case 2:
		SetChrBank2k(bankA, CHR_0000, banks[1]);
		SetChrBank2k(bankA, CHR_0800, banks[3]);
		SetChrBank2k(bankA, CHR_1000, banks[5]);
		SetChrBank2k(bankA, CHR_1800, banks[7]);

		SetChrBank2k(bankB, CHR_0000, banks[1+8]);
		SetChrBank2k(bankB, CHR_1000, banks[1+8]);
		SetChrBank2k(bankB, CHR_0800, banks[3+8]);
		SetChrBank2k(bankB, CHR_1800, banks[3+8]);
		break;
	case 3:
		SetChrBank1k(bankA, CHR_0000, banks[0]);
		SetChrBank1k(bankA, CHR_0400, banks[1]);
		SetChrBank1k(bankA, CHR_0800, banks[2]);
		SetChrBank1k(bankA, CHR_0C00, banks[3]);
		SetChrBank1k(bankA, CHR_1000, banks[4]);
		SetChrBank1k(bankA, CHR_1400, banks[5]);
		SetChrBank1k(bankA, CHR_1800, banks[6]);
		SetChrBank1k(bankA, CHR_1C00, banks[7]);


		SetChrBank1k(bankB, CHR_0000, banks[8]);
		SetChrBank1k(bankB, CHR_0400, banks[9]);
		SetChrBank1k(bankB, CHR_0800, banks[10]);
		SetChrBank1k(bankB, CHR_0C00, banks[11]);
		SetChrBank1k(bankB, CHR_1000, banks[8]);
		SetChrBank1k(bankB, CHR_1400, banks[9]);
		SetChrBank1k(bankB, CHR_1800, banks[10]);
		SetChrBank1k(bankB, CHR_1C00, banks[11]);

		break;

	}

	switch (prgMode & 0x03)
	{
	case 0:
		SetPrgBank32k(prg_reg[3] | 0x80);
		break;
	case 1:
		SetPrgBank16k(PRG_8000, prg_reg[1]);
		SetPrgBank16k(PRG_C000, prg_reg[3] | 0x80);
		break;
	case 2:
		SetPrgBank16k(PRG_8000, prg_reg[1]);
		SetPrgBank8k(PRG_C000, prg_reg[2]);
		SetPrgBank8k(PRG_E000, prg_reg[3] | 0x80);
		break;
	case 3:
		SetPrgBank8k(PRG_8000, prg_reg[0]);
		SetPrgBank8k(PRG_A000, prg_reg[1]);
		SetPrgBank8k(PRG_C000, prg_reg[2]);
		SetPrgBank8k(PRG_E000, prg_reg[3] | 0x80);
		break;
	}
}

unsigned char c_mapper5::ReadByte(unsigned short address)
{
	switch (address >> 12)
	{
	case 5:
		if (address >= 0x5C00 && address < 0x6000)
		{
			if (exram_mode > 1)
				return exram[address - 0x5C00];
			else
				return 0;
		}
		switch (address)
		{
		case 0x5204:
			{
				int val = (inFrame << 6) | (irqPending << 7);
				irqPending = 0;
				if (irq_asserted)
				{
					cpu->clear_irq();
					irq_asserted = 0;
				}
				return val;
			}
		case 0x5205:
			return (multiplicand * multiplier) & 0xFF;
		case 0x5206:
			return ((multiplicand * multiplier) >> 8) & 0xFF;
		default:
			return 0;
		}
		break;
	case 6:
	case 7:
		return *(prg_6000 + (address & 0x1FFF));
	default:
		return c_mapper::ReadByte(address);
	}

	//return c_mapper::ReadByte(address);
}

void c_mapper5::reset(void)
{
	prg_reg[0] = 0;
	prg_reg[1] = 0;
	prg_reg[2] = 0;
	prg_reg[3] = 0xFF;
	prg_6000 = prg_ram;
	prg_ram_protect[0] = 0;
	prg_ram_protect[1] = 0;
	exram_mode = 0;
	chr_high = 0;
	multiplicand = 0;
	multiplier = 0;
	last_tile = 0;

	exramTileSelect = 0;
	exramColorSelect = 0;
	nt2000 = 0;
	nt2400 = 0;
	nt2800 = 0;
	nt2c00 = 0;
	splitTile = 0;
	splitSide = 0;
	splitEnable = 0;
	fillTile = 0;
	fillColor = 0;
	prgMode = 3;
	chrMode = 0;
	irqPending = 0;
	irqCounter = 0;
	inFrame = 0;
	irqEnable = 0;
	irqTarget = 0;
	lastBank = 0;
	SetChrBank8k(bankA, 0);
	SetChrBank8k(bankB, 0);
	Sync();
	irq_asserted = 0;
	using_fill_table = 0;
	drawing_enabled = 0;
	split_control = 0;
	split_scroll = 0;
	split_page = 0;
	cycle = 0;
	scanline = 0;
	scroll_line = 0;
	cycles_since_rendering = 0;
	read_buffer = 0;
	last_ppu_read = 0;
	fetch_count = 0;
	irq_q = 0;
	vscroll = 0;
}

void c_mapper5::clock(int cycles)
{
	for (int i = 0; i < cycles; i++)
	{
		if (cycle == 0)
		{
			if (scanline == 20)
			{
				vscroll = split_scroll;
			}

			if (scanline >= 21 && scanline < 261 && drawing_enabled)
			{
				if (inFrame == 0)
				{
					inFrame = 1;
					irqCounter = 0;
					irqPending = 0;
					if (irq_asserted)
					{
						cpu->clear_irq();
						irq_asserted = 0;
					}
				}
				else
				{
					irqCounter++;
					if (irqCounter == irqTarget)
					{
						irqPending = 1;
						if (irqEnable && !irq_asserted)
						{
							cpu->execute_irq();
							irq_asserted = 1;
						}
					}
				}
			}
		}
		else if (cycle == 320)
		{
			if (scanline >= 21 && scanline < 261 && drawing_enabled)
			{
				vscroll++;
				if (vscroll > 239)
					vscroll -= 240;
			}
		}
		if (++cycle == 341)
		{
			if (scanline == 260)
				inFrame = 0;
			cycle = 0;
			if (++scanline == 262)
				scanline = 0;
		}
	}
}