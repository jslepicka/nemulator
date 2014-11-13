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

#include "nes.h"
#include "apu.h"
#include ".\Nes_Snd_Emu\nes_apu.h"
#include ".\Nes_Snd_Emu\sound_writer.hpp"

c_apu::c_apu(void)
{
	apu = new Nes_Apu();
	buf = new Blip_Buffer();
	output = new blip_sample_t[sampleBufferSize];
	samples = 0;
	buf->sample_rate(48000);
	buf->clock_rate(baserate);
	apu->output(buf);
}

c_apu::~c_apu(void)
{
	if (apu)
		delete apu;
	if (buf)
		delete buf;
	if (output)
		delete output;
}

unsigned char c_apu::ReadByte(unsigned short address)
{
	return apu->read_status(0);
}

void c_apu::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= apu->start_addr && address <= apu->end_addr)
		apu->write_register(0, address, value);
}

void c_apu::EndLine(int cycles)
{
	if (cycles == 0)
		return;
	apu->end_frame(cycles);
	buf->end_frame(cycles);
}

int c_apu::EndFrame(void)
{
	//apu->end_frame(29829);
	//buf->end_frame(29829);
	int x = buf->samples_avail();
	samples = buf->read_samples(output, sampleBufferSize);
	return 0;
}

void c_apu::Reset(void)
{
	apu->reset();
	apu->SetNes(nes);
	
}