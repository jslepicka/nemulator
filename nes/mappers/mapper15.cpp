#include "mapper15.h"


c_mapper15::c_mapper15()
{
	//100-in-1 Contra Function 16
	mapperName = "Mapper 15";
}

void c_mapper15::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		mode = address & 0x03;
		reg = value;
		sync();
	}
	else
		c_mapper::WriteByte(address, value);
}

void c_mapper15::reset()
{
	reg = 0;
	mode = 0;
}

void c_mapper15::sync()
{
	int banks[4];
	int bank = (reg & 0x3F) << 1;
	int flip = reg >> 7;
	switch (mode)
	{
	case 0:
		bank &= 0x7C;
		banks[0] = bank | 0 ^ flip;
		banks[1] = bank | 1 ^ flip;
		banks[2] = bank | 2 ^ flip;
		banks[3] = bank | 3 ^ flip;
		break;
	case 1:
		banks[0] = bank | 0 ^ flip;
		banks[1] = bank | 1 ^ flip;
		banks[2] = 0x7E | 0 ^ flip;
		banks[3] = 0x7F | 1 ^ flip;
		break;
	case 2:
		banks[0] = bank ^ flip;
		banks[1] = bank ^ flip;
		banks[2] = bank ^ flip;
		banks[3] = bank ^ flip;
		break;
	case 3:
		banks[0] = bank | 0 ^ flip;
		banks[1] = bank | 1 ^ flip;
		banks[2] = bank | 0 ^ flip;
		banks[3] = bank | 1 ^ flip;
		break;
	}
	for (int i = PRG_8000; i <= PRG_E000; i++)
		SetPrgBank8k(i, banks[i]);
	if (reg & 0x40)
		set_mirroring(MIRRORING_HORIZONTAL);
	else
		set_mirroring(MIRRORING_VERTICAL);
}