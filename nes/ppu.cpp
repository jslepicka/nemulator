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

//#include "nes.h"
#include "cpu.h"
#include "ppu.h"
#include "memory.h"
#include "mapper.h"
#include "apu2.h"
#include <new>
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>


#include <crtdbg.h>
//#if defined(DEBUG) | defined(_DEBUG)
//#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
//#define new DEBUG_NEW
//#endif

std::atomic<int> c_ppu::lookup_tables_built = 0;
uint8_t c_ppu::attr_shift_table[0x400];
uint8_t c_ppu::attr_loc[0x400];
uint64_t c_ppu::morton_odd_64[256];
uint64_t c_ppu::morton_even_64[256];

uint32_t c_ppu::pal[512];

c_ppu::c_ppu(void)
{
	pSpriteMemory = 0;
	mapper = 0;
	build_lookup_tables();
	limit_sprites = false; //don't change this on reset
}

void c_ppu::generate_palette()
{
	double saturation = 1.3;
	double hue_tweak = 0.0;
	double contrast = 1.0;
	double brightness = 1.0;

	for (int pixel = 0; pixel < 512; pixel++)
	{
		int color = (pixel & 0xF);
		int level = color < 0xE ? (pixel >> 4) & 0x3 : 1;
		
		static const double black = .518;
		static const double white = 1.962;
		static const double attenuation = .746;
		static const double levels[8] = { .350, .518, .962, 1.550,
			1.094, 1.506, 1.962, 1.962 };
		double lo_and_hi[2] = { levels[level + 4 * (color == 0x0)],
			levels[level + 4 * (color < 0xd)] };
		double y = 0.0;
		double i = 0.0;
		double q = 0.0;

		for (int p = 0; p < 12; ++p)
		{
			auto wave = [](int p, int color) {
				return (color + p + 8) % 12 < 6;
			};

			double spot = lo_and_hi[wave(p, color)];
			if (((pixel & 0x40) && wave(p, 12))
				|| ((pixel & 0x80) && wave(p, 4))
				|| ((pixel & 0x100) && wave(p, 8))) spot *= attenuation;
			double v = (spot - black) / (white - black);

			v = (v - .5) * contrast + .5;
			v *= brightness / 12.0;

			y += v;
			i += v * cos((M_PI / 6.0) * (p + hue_tweak));
			q += v * sin((M_PI / 6.0) * (p + hue_tweak));
		}

		i *= saturation;
		q *= saturation;

		//Y'IQ -> NTSC R'G'B'
		//Adapted from http://en.wikipedia.org/wiki/YIQ, FCC matrix []^-1
		double ntsc_r = y + 0.956 * i + 0.620 * q;
		double ntsc_g = y + -0.272 * i + -0.647 * q;
		double ntsc_b = y + -1.108 * i + 1.705 * q;

		//NTSC R'G'B' -> linear NTSC RGB
		ntsc_r = pow(std::clamp(ntsc_r, 0.0, 1.0), 2.2);
		ntsc_g = pow(std::clamp(ntsc_g, 0.0, 1.0), 2.2);
		ntsc_b = pow(std::clamp(ntsc_b, 0.0, 1.0), 2.2);

		//NTSC RGB (SMPTE-C) -> CIE XYZ
		//conversion matrix from http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
		double cie_x = ntsc_r * 0.3935891 + ntsc_g * 0.3652497 + ntsc_b * 0.1916313;
		double cie_y = ntsc_r * 0.2124132 + ntsc_g * 0.7010437 + ntsc_b * 0.0865432;
		double cie_z = ntsc_r * 0.0187423 + ntsc_g * 0.1119313 + ntsc_b * 0.9581563;

		//CIE XYZ -> linear sRGB
		//Shader will return sR'G'B'
		//conversion matrix from http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
		double srgb_r2 = cie_x * 3.2404542 + cie_y * -1.5371385 + cie_z * -0.4985314;
		double srgb_g2 = cie_x * -0.9692660 + cie_y * 1.8760108 + cie_z * 0.0415560;
		double srgb_b2 = cie_x * 0.0556434 + cie_y * -0.2040259 + cie_z * 1.0572252;

		//linear RGB -> sRGB
		
		srgb_r2 = std::clamp(pow(srgb_r2, 1.0 / 2.2), 0.0, 1.0);
		srgb_g2 = std::clamp(pow(srgb_g2, 1.0 / 2.2), 0.0, 1.0);
		srgb_b2 = std::clamp(pow(srgb_b2, 1.0 / 2.2), 0.0, 1.0);

		int rgb = 0x000001 * (int)(255.0 * srgb_r2)
			+ 0x000100 * (int)(255.0 * srgb_g2)
			+ 0x010000 * (int)(255.0 * srgb_b2);

		pal[pixel] = rgb | 0xFF000000;
	}
}

void c_ppu::build_lookup_tables()
{
	int expected = 0;
	if (lookup_tables_built.compare_exchange_strong(expected, 1))
	{
		for (int i = 0; i < 0x400; ++i)
		{
			attr_shift_table[i] = ((i >> 4) & 0x04) | (i & 0x02);
			attr_loc[i] = ((i >> 4) & 0x38) | ((i >> 2) & 0x07);
		}

		for (int i = 0; i < 256; i++)
		{
			__int64 result_64 = 0;
			int temp = i;
			for (int b = 0; b < 8; b++)
			{
				result_64 <<= 8;
				result_64 |= (temp & 1);
				temp >>= 1;
			}
			morton_odd_64[i] = result_64;
			morton_even_64[i] = result_64 << 1;
		}

		generate_palette();
	}
}

c_ppu::~c_ppu(void)
{
	if (pSpriteMemory)
		delete[] pSpriteMemory;
}

__forceinline uint64_t c_ppu::interleave_bits_64(unsigned char odd, unsigned char even)
{
	return morton_odd_64[odd] | morton_even_64[even];
}
unsigned char c_ppu::read_byte(int address)
{
	unsigned char return_value = 0;
	switch (address)
	{
	case 0x2000:    //PPU Control Register 1
	{
		return_value = *control1;
		break;
	}
	case 0x2001:    //PPU Control Register 2
	{
		return_value = *control2;
		break;
	}
	case 0x2002:    //PPU Status Register
	{
		hi = false;
		unsigned char temp = *status;
		if (current_scanline == 241) {
			switch (current_cycle) {
			case 1:
				temp &= ~0x80;
				suppress_nmi = 1;
				cpu->clear_nmi();
				break;
			case 2:
			case 3:
				temp |= 0x80;
				cpu->clear_nmi();
				break;
			default:
				break;
			}
		}
		ppuStatus.vBlank = false;
		return_value = (unsigned char)((temp & 0xE0) | (vramAddressLatch & 0x1F));

		break;
	}
	case 0x2003:
	{
		return_value = 0;
		break;
	}
	case 0x2004:    //Sprite Memory Data
	{
		//return sprite memory
		if (rendering)
		{
			if (current_cycle == 0 || current_cycle >= 320) {
				return_value = sprite_buffer[0];
			}
			else if (current_cycle >= 1 || current_cycle <= 64)
				return_value = 0xFF;
			else if (current_cycle >= 65 || current_cycle <= 256)
				return_value = 0x00; //this isn't correct, but I don't think anything relies on it
			else if (current_cycle >= 257 || current_cycle <= 320)
				return_value = 0xFF;
			else
				return_value = sprite_buffer[0];
		}
		else
			return_value = pSpriteMemory[spriteMemAddress & 0xFF];
		break;
	}
	case 0x2007:    //PPU Memory Data
	{
		unsigned char temp = readValue;
		if (!rendering)
			readValue = mapper->ppu_read(vramAddress);

		//palette reads are returned immediately
		if ((vramAddress & 0x3FFF) >= 0x3F00)
			temp = image_palette[vramAddress & 0x1F];


		update_vram_address();

		return_value = temp;
		break;
	}
	default:
		return_value = 0;
	}
	return return_value;
}

void c_ppu::inc_horizontal_address()
{
	if ((vramAddress & 0x1F) == 0x1F)
		vramAddress ^= 0x41F;
	else
		vramAddress++;
}

void c_ppu::write_byte(int address, unsigned char value)
{
	switch (address)
	{
	case 0x2000:    //PPU Control Register 1
	{
		//if nmi enabled is false and incoming value enables it
		//AND if currently in NMI, then execute_nmi
		if (!(*control1 & 0x80) && (value & 0x80) && ppuStatus.vBlank)
			cpu->execute_nmi();
		if (current_scanline == 261 && (*control1 & 0x80) && current_cycle < 4) {
			cpu->clear_nmi();
		}

		*control1 = value;
		if (ppuControl1.verticalWrite)
		{
			addressIncrement = 32;
		}
		else
		{
			addressIncrement = 1;
		}
		vramAddressLatch &= 0x73FF;
		vramAddressLatch |= ((value & 3) << 10);
		break;
	}
	case 0x2001:    //PPU Control Register 2
	{
		*control2 = value;
		sprites_visible = ppuControl2.spritesVisible;
		if ((value & 0x18) && (current_scanline < 240 || current_scanline == 261)) {
			next_rendering = 1;
			//Battletoads debugging
			//char x[256];
			//sprintf(x, "enabled rendering at scanline %d, cycle %d\n", current_scanline, current_cycle);
			//OutputDebugString(x);
		}
		else {
			next_rendering = 0;
		}
		if (next_rendering != rendering) {
			update_rendering = 1;
		}

		if (value & 0x01)
		{
			palette_mask = 0x30;
		}
		else
			palette_mask = 0xFF;
		intensity = (value & 0xE0) << 1;
		break;
	}
	case 0x2002:    //PPU Status Register
	{
		//*status = value;
		int x = 1;
		break;
	}
	case 0x2003:    //Sprite Memory Address
	{
		spriteMemAddress = value;
		break;
	}
	case 0x2004:    //Sprite Memory Data
	{
		*(pSpriteMemory + spriteMemAddress) = value;
		spriteMemAddress++;
		spriteMemAddress &= 0xFF;
		break;
	}
	case 0x2005:    //Background Scroll
	{
		if (!hi)
		{
			vramAddressLatch &= 0x7FE0;
			vramAddressLatch |= ((value & 0xF8) >> 3);
			fineX = (value & 0x07);
		}
		else
		{
			vramAddressLatch &= 0x0C1F;
			vramAddressLatch |= ((value & 0xF8) << 2);
			vramAddressLatch |= ((value & 0x07) << 12);
		}
		hi = !hi;
		break;
	}
	case 0x2006:    //PPU Memory Address
	{
		if (!hi)
		{
			vramAddressLatch &= 0x00FF;
			vramAddressLatch |= ((value & 0x3F) << 8);
		}
		else
		{
			vramAddressLatch &= 0x7F00;
			vramAddressLatch |= value;
			vram_update_delay = VRAM_UPDATE_DELAY;
		}
		hi = !hi;
		break;
	}
	case 0x2007:    //PPU Memory Data
	{
		if ((vramAddress & 0x3F00) == 0x3F00)
		{
			if (!rendering) {
				int pal_address = vramAddress & 0x1F;
				value &= 0x3F;
				image_palette[pal_address] = value;
				if (!(pal_address & 0x03))
				{
					image_palette[pal_address ^ 0x10] = value;
				}
				mapper->ppu_read(vramAddress);
			}
		}
		else
		{
			if (!rendering) {
				mapper->ppu_write(vramAddress, value);
			}
		}
		update_vram_address();
	}
	break;
	}
}

void c_ppu::inc_vertical_address()
{

	if ((vramAddress & 0x7000) == 0x7000)
	{
		int t = vramAddress & 0x3E0;
		vramAddress &= 0xFFF;
		switch (t)
		{
		case 0x3A0:
			vramAddress ^= 0xBA0;
			break;
		case 0x3E0:
			vramAddress ^= 0x3E0;
			break;
		default:
			vramAddress += 0x20;
		}
	}
	else
		vramAddress += 0x1000;
}

void c_ppu::reset(void)
{
	warmed_up = 0;
	odd_frame = 0;
	intensity = 0;
	palette_mask = 0xFF;
	current_cycle = 0;
	nmi_pending = false;
	current_scanline = 0;
	rendering = 0;
	pattern_address = attribute_address = 0;
	on_screen = 0;
	control1 = (unsigned char*)&ppuControl1;
	control2 = (unsigned char*)&ppuControl2;
	status = (unsigned char*)&ppuStatus;
	if (!pSpriteMemory)
		pSpriteMemory = new unsigned char[256];
	memset(pSpriteMemory, 0, 256);
	pFrameBuffer = (int*)&frameBuffer;
	memset(pFrameBuffer, 0, 256 * 256 * sizeof(int));
	readValue = 0;
	*control1 = 0;
	*control2 = 0;
	*status = 0;
	hi = false;
	spriteMemAddress = 0;
	addressIncrement = 1;
	vramAddress = vramAddressLatch = fineX = 0;
	drawingBg = 0;
	executed_cycles = 3;
	pattern1 = pattern2 = 0;

	pattern_address = attribute_address = 0;

	memset(index_buffer, 0xFF, sizeof(index_buffer));
	memset(image_palette, 0x3F, 32);
	memset(sprite_buffer, 0, sizeof(sprite_buffer));

	sprites_visible = 0;
	fetch_state = FETCH_IDLE;
	vram_update_delay = 0;
	suppress_nmi = 0;
	hit = 0;
	update_rendering = 0;
	next_rendering = 0;

	pixel_pipeline = 0;
}

void c_ppu::run_ppu_line()
{
	//current_cycle = 0;
	int pixel_count = 0;
	int done = 0;
	const int screen_offset = 2;
	int vid_out = 0;

	int reload_v = 0;

	while (true)
	{
		cpu->add_cycle();

		if (current_cycle != 0) [[likely]] {
			switch (current_cycle) {

			case 1:
				if (current_scanline == 0) [[unlikely]] {
					p_frame = pFrameBuffer;
					hit = 0;
				}
				else if (current_scanline == 240) [[unlikely]] {
					rendering = 0;
				}
				else if (current_scanline == 241) [[unlikely]] {
					if (warmed_up) {
						if (!suppress_nmi) {
							ppuStatus.vBlank = true;
							if (ppuControl1.vBlankNmi) {
								cpu->execute_nmi();
							}
						}
						else {
							ppuStatus.vBlank = false;
						}
					}
					suppress_nmi = 0;
				}
				else if (current_scanline == 261) [[unlikely]] {
					rendering = drawing_enabled();
					ppuStatus.spriteCount = false;
					//ppuStatus.vBlank = false;
					ppuStatus.hitFlag = false;
				}
				fetch_state = FETCH_BG;
				break;
			case 2:
				//this technically happens on cycle 1, but if the cpu reads it then, it should still read back as true
				//therefore, clearing the flag here.
				if (current_scanline == 261) {
					ppuStatus.vBlank = false;
					cpu->clear_nmi();
				}
				//screen_offset
				if (current_scanline < 240) {
					on_screen = 1;
				}
				break;
			case 4:
				//enable pixel output
				if (current_scanline < 240) {
					vid_out = 1;
				}
				break;
			case 257:
				fetch_state = FETCH_SPRITE;
				break;
			case 258: //256 + screen_offset
				on_screen = 0;
				break;
			case 260:
				//disable pixel output
				vid_out = 0;
				break;
			case 280:
				if (current_scanline == 261) {
					reload_v = 1;
				}
				break;
			case 305:
				if (current_scanline == 261) {
					reload_v = 0;
				}
				break;
			case 321:
				fetch_state = FETCH_BG;
				break;
			case 337:
				fetch_state = FETCH_NT;
				break;
			default:
				break;
			}

			if (rendering) [[likely]] {
				if (reload_v) [[unlikely]] {
					vramAddress = (vramAddress & ~0x7BE0) | (vramAddressLatch & 0x7BE0);
				}
				unsigned int s = (((current_cycle - 1) & 0x7) | fetch_state)/* & 0x1F*/;
				switch (s) {
				case (0 | FETCH_BG): //NT byte 1
				case (0 | FETCH_NT):
					break;
				case (0 | FETCH_SPRITE):
					if (current_cycle == 257) {
						vramAddress &= ~0x41F;
						vramAddress |= (vramAddressLatch & 0x41F);
					}
					break;
				case (1 | FETCH_BG): //NT byte 2
					drawingBg = true;
					tile = mapper->ppu_read(0x2000 | (vramAddress & 0xFFF));
					pattern_address = (tile << 4) + ((vramAddress & 0x7000) >> 12) + (ppuControl1.screenPatternTableAddress * 0x1000);
					break;
				case (1 | FETCH_SPRITE):
					//tile = mapper->ppu_read(0x2000 | (vramAddress & 0xFFF));
					break;
				case (1 | FETCH_NT):
					tile = mapper->ppu_read(0x2000 | (vramAddress & 0xFFF));
					break;
				case (2 | FETCH_BG): //AT byte 1
				case (2 | FETCH_SPRITE):
				case (2 | FETCH_NT):
					break;
				case (3 | FETCH_BG): //AT byte 2
					attribute_address = 0x23C0 |
						(vramAddress & 0xC00) |
						(attr_loc[vramAddress & 0x3FF]);
					attribute_shift = attr_shift_table[vramAddress & 0x3FF];
					attribute = mapper->ppu_read(attribute_address);
					attribute >>= attribute_shift;
					attribute &= 0x03;
					break;
				case (3 | FETCH_SPRITE):
					//tile = mapper->ppu_read(0x2000 | (vramAddress & 0xFFF));
					break;
				case (3 | FETCH_NT):
					tile = mapper->ppu_read(0x2000 | (vramAddress & 0xFFF));
					break;
				case (4 | FETCH_BG): //Low BG byte 1
				case (4 | FETCH_SPRITE):
				case (4 | FETCH_NT):
					break;
				case (5 | FETCH_BG): //Low BG byte 2
					drawingBg = true;
					pattern1 = mapper->ppu_read(pattern_address);
					break;
				case (5 | FETCH_SPRITE):
					drawingBg = false;
					if (ppuControl1.spriteSize)
					{
						int sprite_tile = sprite_buffer[(((current_cycle - 261) / 8) * 4) + 1];
						mapper->ppu_read((sprite_tile & 0x1) * 0x1000);
					}
					else
					{
						mapper->ppu_read(ppuControl1.spritePatternTableAddress * 0x1000);
					}
					break;
				case (5 | FETCH_NT):
					break;
				case (6 | FETCH_BG): //High BG byte 1
				case (6 | FETCH_SPRITE):
				case (6 | FETCH_NT):
					break;
				case (7 | FETCH_BG): //High BG byte 2
					drawingBg = true;
					pattern2 = mapper->ppu_read(pattern_address + 8);
					{
						unsigned int l = current_cycle + 8;

						if (l >= 336)
							l -= 336;
						uint64_t* ib64 = (uint64_t*)&index_buffer[l];
						uint64_t c = interleave_bits_64(pattern1, pattern2) | (attribute * 0x0404040404040404ULL);
						*ib64 = c;
					}
					if (current_cycle == 256) {
						inc_vertical_address();
					}
					inc_horizontal_address();

					break;
				case (7 | FETCH_SPRITE):
					drawingBg = false;
					if (ppuControl1.spriteSize)
					{
						int sprite_tile = sprite_buffer[(((current_cycle - 261) / 8) * 4) + 1];
						mapper->ppu_read((sprite_tile & 0x1) * 0x1000);
					}
					else
					{
						mapper->ppu_read(ppuControl1.spritePatternTableAddress * 0x1000);
					}
					break;
				case (7 | FETCH_NT):
					break;
				default:
					__assume(0);
				}
				if (on_screen) [[likely]]
				{
					int pixel = 0;
					int bg_index = 0;

					if (ppuControl2.backgroundSwitch && ((current_cycle >= 8 + screen_offset) || ppuControl2.backgroundClipping))
					{
						bg_index = index_buffer[current_cycle - screen_offset + fineX];
					}

					if (bg_index & 0x03)
					{
						pixel = image_palette[bg_index];
					}
					else
					{
						pixel = image_palette[0];
					}

					if (sprite_count && ppuControl2.spritesVisible && ((current_cycle >= 8 + screen_offset) || ppuControl2.spriteClipping)) [[unlikely]]
					{
						int max_sprites = limit_sprites ? 8 : 64;
						for (int i = 0; i < sprite_count && i < max_sprites; i++)
						{
							int sprite_x = sprite_buffer[(i * 4) + 3];

							if ((unsigned int)(current_cycle - screen_offset - sprite_x) < 8)
							{
								int priority = sprite_buffer[(i * 4) + 2] & 0x20;
								int sprite_color = sprite_index_buffer[i * 8 + (current_cycle - screen_offset - sprite_x)];
								if (sprite_color & 0x03)
								{
									if (i == sprite0_index &&
										ppuControl2.backgroundSwitch &&
										(bg_index & 0x03) != 0 &&
										current_cycle < 255 + screen_offset) [[unlikely]] {
										ppuStatus.hitFlag = true;
										hit = 1;
									}

									if (!(priority && (bg_index & 0x03))) {
										pixel = image_palette[16 + sprite_color];
									}
									break;
								}
							}
						}
					}
					pixel_pipeline |= (pixel << 16);
				}
			}

			else if (on_screen)
			{
				int pixel = 0;
				if ((vramAddress & 0x3F00) == 0x3F00)
				{
					pixel = image_palette[vramAddress & 0x1F];
				}
				else
				{
					pixel = image_palette[0];
				}
				pixel_pipeline |= (pixel << 16);
			}

			if (current_cycle == 340) [[unlikely]]
			{
				if (current_scanline == 261) [[unlikely]]
				{
					current_scanline = 0;
					warmed_up = 1;
					current_cycle = (rendering && odd_frame) ? 1 : 0;
					odd_frame ^= 1;
					if (hit == 0) {
						int x = 1;
					}
				}
				else {
					current_cycle = 0;
					current_scanline++;
				}
				done = 1;
			}
		}
			//idle:
				if (vid_out /*current_scanline < 240 && current_cycle >= 4 && current_cycle < 4 + 256*/) [[likely]] {
					*p_frame++ = pal[(pixel_pipeline & palette_mask) | intensity];
				}
			pixel_pipeline >>= 8;
			//Battletoads will enable rendering at cycle 255 and break sprite 0 hit (because vertical update happens on cycle 256 and misaligns the background).
			//Delaying the update by one cycle seems to fix this.  Unsure if this is actually how it works, but Mesen source code says that this is what
			//apparently happens.  Need to research how enabling things mid-screen affects PPU state.
			//
			//NMI at scanline 241, cycle 28
			//enabled rendering at scanline 14, cycle 300
			//enabled rendering at scanline 15, cycle 316
			//NMI at scanline 241, cycle 29
			//enabled rendering at scanline 14, cycle 271
			//enabled rendering at scanline 15, cycle 287
			//NMI at scanline 241, cycle 30
			//enabled rendering at scanline 14, cycle 293
			//enabled rendering at scanline 15, cycle 309
			//NMI at scanline 241, cycle 25
			//enabled rendering at scanline 14, cycle 255 <-- breaks here

			if (update_rendering) [[unlikely]] {
				rendering = next_rendering;
				update_rendering = 0;
			}
				//TODO: all cpu writes affecting ppu state should probably be delayed until next ppu cycle
				if (vram_update_delay > 0) [[unlikely]] {
					vram_update_delay--;
					if (vram_update_delay == 0) {
						vramAddress = vramAddressLatch;
						if (!rendering)
						{
							mapper->ppu_read(vramAddress);
						}
					}
				}

			mapper->clock(1);
			if (--executed_cycles == 0) [[unlikely]]
			{
				cpu->execute();
				cpu->odd_cycle ^= 1;
				apu2->clock_once();
				executed_cycles = 3;
			}
				if (done) {
					return;
				}
			current_cycle++;
	}
}

bool c_ppu::drawing_enabled(void)
{
	if (ppuControl2.backgroundSwitch || ppuControl2.spritesVisible)
		return true;
	else
		return false;
}

int c_ppu::get_sprite_size()
{
	return (int)ppuControl1.spriteSize;
}

int c_ppu::eval_sprites()
{
	if (!rendering)
		return -1;
	drawingBg = false;
	mapper->in_sprite_eval = 1;
	int max_sprites = limit_sprites ? 8 : 64;
	int sprite0_x = -1;
	sprite0_index = -1;
	sprite_count = 0;
	memset(sprite_buffer, 0xFF, sizeof(sprite_buffer));
	int sprite_line = current_scanline;
	for (int i = 0; i < 64/* && sprite_count < max_sprites*/; i++)
	{
		int sprite_offset = i * 4;
		int y = *(pSpriteMemory + sprite_offset) + 1;
		int sprite_height = 8 << (int)ppuControl1.spriteSize;
		if (((sprite_line) >= y) && ((sprite_line) < (y + sprite_height)))
		{
			if (i == 0)
			{
				sprite0_x = *(pSpriteMemory + sprite_offset + 3);
				sprite0_index = sprite_count;
			}
			memcpy(&sprite_buffer[sprite_count * 4], pSpriteMemory + sprite_offset, 4);


			int sprite_tile = *(pSpriteMemory + sprite_offset + 1);
			int sprite_y = (sprite_line)-y;
			int sprite_attribute = *(pSpriteMemory + sprite_offset + 2);

			if (sprite_attribute & 0x80)
			{
				sprite_y = (sprite_height - 1) - sprite_y;
			}

			int sprite_address = 0;

			if (sprite_height == 16)
			{
				sprite_address = ((sprite_tile & 0xFE) * 16) + (sprite_y & 0x07) + ((sprite_tile & 0x01) * 0x1000);
				if (sprite_y > 7) sprite_address += 16;
			}
			else
				sprite_address = (sprite_tile * 16) + (sprite_y & 0x07) + (ppuControl1.spritePatternTableAddress * 0x1000);

			int attr = ((sprite_attribute & 0x03) << 2) * 0x01010101;
			int* p1 = (int*)morton_odd_64 + mapper->ppu_read(sprite_address) * 2;
			int* p2 = (int*)morton_even_64 + mapper->ppu_read(sprite_address + 8) * 2;

			int p[2] = { *p1++ | *p2++ | attr, *p1 | *p2 | attr };

			if (sprite_attribute & 0x40)
			{
				unsigned char* sb = &sprite_index_buffer[sprite_count * 8];
				unsigned char* pc = (unsigned char*)p + 7;
				for (int j = 0; j < 8; j++)
					*sb++ = *pc--;
			}
			else
			{
				int* sb = (int*)&sprite_index_buffer[sprite_count * 8];
				for (int j = 0; j < 2; j++)
					*sb++ = p[j];
			}

			sprite_count++;
			if (sprite_count == 9)
				ppuStatus.spriteCount = true;
		}
	}
	mapper->in_sprite_eval = 0;
	if (current_scanline == 261)
		sprite_count = 0;
	return sprite0_x;
}

void c_ppu::update_vram_address()
{
	if (rendering)
	{
		inc_horizontal_address();
		inc_vertical_address();
	}
	else
	{
		//update bus when not rendering
		vramAddress = (vramAddress + addressIncrement) & 0x7FFF;
		mapper->ppu_read(vramAddress);
	}
}