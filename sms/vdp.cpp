#include "vdp.h"
#include "sms.h"
#include <memory>
#include <algorithm>

#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

std::atomic<int> c_vdp::pal_built = 0;
uint32_t c_vdp::pal_sms[256];
uint32_t c_vdp::pal_gg[4096];

c_vdp::c_vdp(c_sms* sms, int type)
{
	this->sms = sms;
	mode = type;
	vram = new unsigned char[16384];
	frame_buffer = new int[256 * 256];
	generate_palette();
}


c_vdp::~c_vdp(void)
{
	delete[] frame_buffer;
	delete[] vram;
}

void c_vdp::reset()
{
	sprite_count = 0;
	line_number = 0;
	line_irq = 0;
	frame_irq = 0;
	line_counter = 255;
	control = 0;
	address = 0;
	address_latch_lo = 0;
	address_latch_hi = 0;
	address_flip_flop = 0;
	read_buffer = 0;
	vram_write = 0;
	memset(registers, 0, sizeof(registers));
	//registers[0x1] |= 0x20;
	//registers[10] = 0xFF;
	//registers[6] = 0xFB;
	memset(vram, 0, 16384);
	memset(cram, 0, sizeof(cram));
	memset(frame_buffer, 0, 256 * 256);
	status = 0;
}

void c_vdp::write_data(unsigned char value)
{
	int a = 1;
	read_buffer = value;
	int cram_mask = mode == MODE_GG ? 0x3F : 0x1F;
	switch (control)
	{
	case 0x03: //CRAM
		cram[address & cram_mask] = value;
		break;
	default: //VRAM
		vram[address & 0x3FFF] = value;
		break;
	}
	address = (address + 1) & 0x3FFF;
	address_flip_flop = 0;
}

unsigned char c_vdp::read_data()
{
	address_flip_flop = 0;
	unsigned char b = read_buffer;
	read_buffer = vram[address++ & 0x3FFF];
	return b;
}

void c_vdp::write_control(unsigned char value)
{
	if (address_flip_flop)
	{
		address_latch_hi = value & 0x3F;
		control = value >> 6;
		address = address_latch_lo | (address_latch_hi << 8);
		switch (control)
		{
		case 0x0:
			read_buffer = vram[address];
			address = (address + 1) & 0x3FFF;
			break;
		case 0x1:
		{
			int x = 1;
		}
		break;
		case 0x2:
		{
			int register_number = ((address >> 8) & 0xF);
			registers[value & 0xF] = address & 0xFF;
			update_irq();
		}
		break;
		case 0x3:
			break;
		}

	}
	else
	{
		address_latch_lo = value;
		address = address_latch_lo | (address_latch_hi << 8);
	}
	address_flip_flop ^= 1;
}

unsigned char c_vdp::read_control()
{
	int ret = status;
	address_flip_flop = 0;
	status &= (~0xE0);
	line_irq = 0;
	frame_irq = 0;
	update_irq();
	return ret;

}

void c_vdp::update_irq()
{
	if (((registers[1] & 0x20) && (status & 0x80)) ||
		((registers[0] & 0x10) && (line_irq)))
	{
		sms->irq = 1;
	}
	else
		sms->irq = 0;
}

void c_vdp::eval_sprites()
{
	sprite_count = 0;
	int sat_base = registers[0x5] & ~0x81;
	sat_base <<= 7;
	if (sat_base != 0)
	{
		int x = 1;
	}
	unsigned char* sat = &vram[sat_base];
	int sprite_height = registers[0x1] & 0x2 ? 16 : 8;
	if (registers[0x1] & 0x1)
	{
		sprite_height *= 2;
	}
	for (int i = 0; i < 64; i++)
	{
		unsigned char sprite_y = *(sat + i);

		if (sprite_y == 0xD0)
			break;
		int sprite_y_adjusted = 0;

		//y values >= 0xF1 are treated as negative
		if (sprite_y >= 0xF1)
			sprite_y_adjusted = (char)sprite_y;
		else
			sprite_y_adjusted = sprite_y;

		sprite_y_adjusted += 1;
		int l = line_number;//;
		int blah = registers[0x1];
		if (sprite_height > 16)
		{
			//printf("big sprite\n");
		}
		if (l >= (sprite_y_adjusted) && l < (sprite_y_adjusted)+sprite_height)
		{
			//sprite is in range
			int sprite_x = *(sat + 128 + i * 2);
			sprite_x -= registers[0] & 0x8;
			int sprite_pattern = *(sat + 129 + i * 2);
			if (registers[0x1] & 0x2)
			{
				sprite_pattern &= ~0x1;
			}
			if (registers[0x6] & 0x4)
			{
				sprite_pattern |= 0x100;
			}
			if (sprite_count < 8)
			{
				sprite_data[sprite_count].x = sprite_x;
				sprite_data[sprite_count].y = sprite_y;
				sprite_data[sprite_count].pattern = sprite_pattern;
				int sprite_base = (registers[0x6] & 0x4) << 10;
				unsigned char* pattern = &vram[/*sprite_base |*/ (sprite_pattern * 32)];
				int yy = l - sprite_y_adjusted;
				if (registers[0x1] & 0x1) //if 8x16 sprite
					yy /= 2;
				pattern += (yy * 4);
				for (int p = 0; p < 8; p++)
				{
					int pal_index = 0;
					for (int ii = 0; ii < 4; ii++)
					{
						pal_index |= ((*(pattern + ii) >> (7 - p)) & 0x1) << ii;
					}
					sprite_data[sprite_count].pixels[p] = pal_index;
				}
				sprite_count++;

			}
			else
			{
				status |= 0x40;
				break;
			}
		}
	}
}

void c_vdp::draw_scanline()
{
	//TODO: test zoomed sprites.  Apparently no SMS games use them?
	//Bock's Birthday 2006
	int sprite_width = registers[0x1] & 0x1 ? 16 : 8;




	int y = line_number;

	if (y < 192)
	{
		if (registers[0x1] & 0x40)
		{
			int x_coarse = registers[8] >> 3;
			int x_fine = registers[8] & 0x7;
			if (y < 16 && registers[0] & 0x40)
			{
				x_coarse = x_fine = 0;
			}
			int y_coarse = registers[9] >> 3;
			int y_fine = registers[9] & 0x7;
			int y_adjusted = y + (y_coarse * 8) + y_fine;
			int y_offset = y_adjusted % 8;
			int y_address = ((y_adjusted / 8) % 28) * 64;
			int background_color = lookup_color((registers[7] & 0xF) | 0x10);
			for (int i = 0; i < 8; i++)
			{
				frame_buffer[y * 256 + i] = background_color;
			}
			for (int column = 0; column < 32; column++)
			{
				unsigned int nt_column = (column - x_coarse) & 0x1F;
				int nt_address = /*((registers[2] & 0xE) << 10) | */(y_address + (nt_column * 2));
				nt_address &= 0x7FF;
				nt_address |= ((registers[2] & 0xE) << 10);
				//if (!(registers[2] & 0x1))
				//{
				//	nt_address &= ~0x400;
				//}
				unsigned char nt_low = vram[nt_address];
				unsigned char nt_hi = vram[nt_address + 1];
				int tile_data = (nt_hi << 8) | nt_low;
				int palette_select = (tile_data & 0x800) >> 7;
				int pattern_number = tile_data & 0x1FF;
				int pattern_base = (registers[4] & 0x7) << 11;
				int h_flip = tile_data & 0x0200;
				int v_flip = tile_data & 0x0400;
				int priority = tile_data & 0x1000;
				unsigned char* pattern = &vram[(pattern_number * 32)];

				int y_offset2 = y_offset;

				if (v_flip)
					y_offset2 = 7 - y_offset2;

				pattern += (y_offset2 * 4);
				int x = ((column * 8) + x_fine) & 0xFF;

				int pat = *((int*)pattern);

				int* fb = &frame_buffer[y * 256 + x];
				for (int p = 0; p < 8 && x < 256; p++, x++)
				{

					//int pal_index = 0;
					int p2 = p;
					if (h_flip)
						p2 = 7 - p2;

					int pat2 = pat >> (7 - p2);
					pat2 &= 0b00000001'00000001'00000001'00000001;
					int pal_index = pat2 | (pat2 >> 7) | (pat2 >> 14) | (pat2 >> 21);
					pal_index &= 0xF;
					//int pal_index = _pext_u32(pat2, 0b00000001'00000001'00000001'00000001);

					//sprites
					int color = 0;
					//if (sprite_count)
					//{
					for (int i = 0; i < sprite_count; i++)
					{
						//int s_x = x;// (column * 8) + p;
						if (x >= sprite_data[i].x && x < sprite_data[i].x + sprite_width)
						{
							int pixel = x - sprite_data[i].x;
							if (sprite_width == 16)
								pixel >>= 1;
							int c = sprite_data[i].pixels[pixel];
							if ((color & 0xF) == 0)
							{
								color = c;
							}
							else
							{
								status |= 0x20;
							}
						}
					}
					//}

					if (mode == MODE_GG && (y < 24 || y >= 168 || x < 48 || x >= 208))
					{
						color = 0;
					}
					else if (x < 8 && (registers[0] & 0x20))
					{
						color = background_color;
					}
					else if ((color & 0xF) == 0 || (priority && pal_index != 0))
					{
						color = lookup_color(pal_index | palette_select);
					}
					else
						color = lookup_color(color | 0x10);

					//frame_buffer[y * 256 + x] = color;
					*fb++ = color;
				}
			}
		}
		else
		{
			for (int x = 0; x < 256; x++)
			{
				frame_buffer[y * 256 + x] = 0;
			}
		}

	}
	else if (y < 256)
	{
		for (int x = 0; x < 256; x++)
		{
			frame_buffer[y * 256 + x] = 0;
		}
	}
	//need to figure out irq timing.
	//leaving line increment here allows earthworm jim (gg) to run but moving the increment past the irq checks
	//causes shacking at the screen split in black belt

	if (line_number > 192)
	{
		line_counter = registers[10];
	}
	else
	{
		line_counter--;
		if (line_counter == 0xFF)
		{
			line_counter = registers[10];
			line_irq = 1;
			update_irq();
		}
	}

	if (line_number == 192)
	{
		status |= 0x80;
		update_irq();

	}
	if (line_number == 261)
		line_number = 0;
	else
		line_number++;


}

int c_vdp::get_scanline()
{
	if (line_number <= 0xDA)
		return line_number;
	else
		return (line_number - 6);
}

int* c_vdp::get_frame_buffer()
{
	return frame_buffer;
}

__forceinline int c_vdp::lookup_color(int palette_index)
{
	switch (mode)
	{
	case MODE_SMS:
		return pal_sms[cram[palette_index]];
	case MODE_GG:
		palette_index <<= 1;
		return pal_gg[
			(cram[palette_index] & 0xFF) |
				((cram[palette_index + 1] & 0xF) << 8)
		];
	default:
		return 0;
	}

}

void c_vdp::generate_palette()
{
	int expected = 0;
	if (pal_built.compare_exchange_strong(expected, 1))
	{
		for (int i = 0; i < 256; i++)
		{
			int r_bits = i & 0x3;
			int g_bits = (i >> 2) & 0x3;
			int b_bits = (i >> 4) & 0x3;

			double r = pow(std::clamp(r_bits * (1.0 / 3.0), 0.0, 1.0), 2.2);
			double g = pow(std::clamp(g_bits * (1.0 / 3.0), 0.0, 1.0), 2.2);
			double b = pow(std::clamp(b_bits * (1.0 / 3.0), 0.0, 1.0), 2.2);

			pal_sms[i] = (int)(255.0 * r)
				| ((int)(255.0 * g) << 8)
				| ((int)(255.0 * b) << 16)
				| 0xFF000000;
		}
		for (int i = 0; i < 4096; i++)
		{
			int r_bits = i & 0xF;
			int g_bits = (i >> 4) & 0xF;
			int b_bits = (i >> 8) & 0xF;

			double r = pow(std::clamp(r_bits * (1.0 / 15.0), 0.0, 1.0), 2.2);
			double g = pow(std::clamp(g_bits * (1.0 / 15.0), 0.0, 1.0), 2.2);
			double b = pow(std::clamp(b_bits * (1.0 / 15.0), 0.0, 1.0), 2.2);

			pal_gg[i] = (int)(255.0 * r)
				| ((int)(255.0 * g) << 8)
				| ((int)(255.0 * b) << 16)
				| 0xFF000000;
		}
	}
}

int c_vdp::get_overscan_color()
{
	switch (mode)
	{
	case MODE_SMS:
		return pal_sms[cram[0x10 | (registers[7] & 0xF)]];
	case MODE_GG:
		//this is wrong
		return pal_gg[cram[0x10 | (registers[7] & 0xF)]];
	default:
		return 0;
	}
}