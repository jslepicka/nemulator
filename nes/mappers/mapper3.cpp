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
#include "mapper3.h"


c_mapper3::c_mapper3(void)
{
	mapperName = "CNROM";
}

c_mapper3::~c_mapper3(void)
{
}

void c_mapper3::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		unsigned char existing_value = ReadByte(address);

		if (existing_value != value)
		{
			int a = 1;
		}
		SetChrBank8k(value & existing_value);
	}
	else
		c_mapper::WriteByte(address, value);

}

void c_mapper3::reset()
{
	c_mapper::reset();
	if (crc32 == 0x6997F5E1)
		this->set_mirroring(MIRRORING_VERTICAL);
}
