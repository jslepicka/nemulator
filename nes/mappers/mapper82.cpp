#include "mapper82.h"


c_mapper82::c_mapper82()
{
	//SD Keiji - Blader
	mapperName = "Mapper 82";
}

void c_mapper82::reset()
{
	chr_mode = 0;
	chr[0] = 0;
	chr[1] = 2;
	chr[2] = 4;
	chr[3] = 5;
	chr[4] = 6;
	chr[5] = 7;
	SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
}

void c_mapper82::sync()
{
	int xor = chr_mode ? 4 : 0;
	SetChrBank2k(CHR_0000 ^ xor, chr[0] >> 1);
	SetChrBank2k(CHR_0800 ^ xor, chr[1] >> 1);
	SetChrBank1k(CHR_1000 ^ xor, chr[2]);
	SetChrBank1k(CHR_1400 ^ xor, chr[3]);
	SetChrBank1k(CHR_1800 ^ xor, chr[4]);
	SetChrBank1k(CHR_1C00 ^ xor, chr[5]);
}

void c_mapper82::WriteByte(unsigned short address, unsigned char value)
{
	if ((address & 0x7EF0) == 0x7EF0)
	{
		switch (address & 0xF)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			chr[address & 0xF] = value;
			sync();
			break;
		case 6:
			chr_mode = value & 0x2;
			if (value & 0x1)
				set_mirroring(MIRRORING_VERTICAL);
			else
				set_mirroring(MIRRORING_HORIZONTAL);
			sync();
			break;
		case 0xA:
			SetPrgBank8k(PRG_8000, value >> 2);
			break;
		case 0xB:
			SetPrgBank8k(PRG_A000, value >> 2);
			break;
		case 0xC:
			SetPrgBank8k(PRG_C000, value >> 2);
			break;
		}
	}
}