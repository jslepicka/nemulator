#include "gbppu.h"
#include "gb.h"
#include "sm83.h"
#include "gbapu.h"
#include <iostream>
#include <assert.h>
#include <utility>

const unsigned int c_gbppu::pal[][4] = {
	{
		//0 - lime green
		//https://www.color-hex.com/color-palette/26401
		0xFF0FBC9B,
		0xFF0FAC8B,
		0xFF306230,
		0xFF0F380F
	},
	{
		//1 - green-yellow from libretro
		//https://docs.libretro.com/library/gambatte/
		0xFF10827B,
		0xFF42795A,
		0xFF4A5939,
		0xFF394129
	},
	{
		//2 - green
		//https://www.deviantart.com/thewolfbunny/art/Game-Boy-Palette-Greenscale-Ver-A-classic-808011585
		0xFF0CBE9C,
		0xFF0A876E,
		0xFF34622C,
		0xFF0C360C
	},
	{
		//3 - brightened green-yellow libretro
		0xFF24968F,
		0xFF568D6E,
		0xFF5A6949,
		0xFF474F37
	},
	{
		//4 - gameboy pocket
		//https://www.deviantart.com/thewolfbunny/art/Game-Boy-Palette-Pocket-Ver-808181843
		0xFFA1CFC4,
		0xFF6D958B,
		0xFF3C534D,
		0xFF1F1F1F
		
	},
	{
		//5 - yellow-green
		//https://lospec.com/palette-list/nostalgia
		0xFF58D0D0,
		0xFF40A8A0,
		0xFF288070,
		0xFF105040
	},
	{
		//6 - DMG
		//https://www.deviantart.com/thewolfbunny/art/Game-Boy-Palette-DMG-Ver-808181265
		0xFF0F867F,
		0xFF447C57,
		0xFF485D36,
		0xFF3B452A
	},
	{
		//7 - black zero
		//http://www.emutalk.net/threads/55441-Game-Boy-Mono-quot-True-quot-colors
		0xFF0F867F,
		0xFF457C57,
		0xFF485D36,
		0xFF3B452A
	},
	{
		//8 - bgb lcd green
		0xFFD0F8E0,
		0xFF70C088,
		0xFF566834,
		0xFF201808
	},
	{
		//9 - shader
		0xFF02988B,
		0xFF027055,
		0xFF02532A,
		0xFF024816
	},
	{
		//10 - RokkumanX
		//https://github.com/libretro/gambatte-libretro/issues/130
		0xFF4FC084,
		0xFF68A66A,
		0xFF68864B,
		0xFF586636
	},
	{
		//11 - Greyscale, gamma adjusted
		0xFFF0F0F0,
		0xFF878787,
		0xFF373737,
		0xFF0C0C0C
	},
	{
		// 12 - Lime Midori
		//https://www.deviantart.com/thewolfbunny/art/Game-Boy-Palette-Lime-Midori-810574708
		0xFFAFEBE0,
		0xFF53CFAA,
		0xFF428D7B,
		0xFF505947,
	},
	{
		//13 - retroarch more contrast
		0xFF1D8F88,
		0xFF4C8364,
		0xFF4A5939,
		0xFF323A22
	},
	{
		//14 - photo
		0xFF08878F,
		0xFF317C63,
		0xFF3C5E35,
		0xFF25442E
	},
	{
		//15 - photo 3x3 sampling
		0xFF078990,
		0xFF337C64,
		0xFF376244,
		0xFF20462C

	}
};

//const int c_gbppu::pal1[] = {
//	0xFF0FBC9B,
//	0xFF0FAC8B,
//	0xFF306230,
//	0xFF0F380F
//};
//
//const int c_gbppu::pal2[] = {
//	0xFF10827B,
//	0xFF42795A,
//	0xFF4A5939,
//	0xFF394129
//};
//
//
//const int c_gbppu::pal3[] = {
//	0xFF0CBE9C,
//	0xFF0A876E,
//	0xFF34622C,
//	0xFF0C360C
//};

std::atomic<int> c_gbppu::color_lookup_built = 0;
uint32_t c_gbppu::color_lookup[32];

c_gbppu::c_gbppu(c_gb* gb)
{
	this->gb = gb;
	vram = new uint8_t[16384];
    vram1 = new uint8_t[16384];
	oam = new uint8_t[160];
	fb = new uint32_t[160 * 144];
	fb_back = new uint32_t[160 * 144];
	shades = pal[15];
    generate_color_lookup();
}

c_gbppu::~c_gbppu()
{
	if (vram)
		delete[] vram;
    if (vram1)
        delete[] vram1;
	if (oam)
		delete[] oam;
	if (fb)
		delete[] fb;
	if (fb_back)
		delete[] fb_back;

}

void c_gbppu::generate_color_lookup()
{
    int expected = 0;
    if (color_lookup_built.compare_exchange_strong(expected, 1)) {
        double gamma = 1.0 / 1.2;
        for (int i = 0; i < 32; i++) {
            double x = i;
			//this curve is based off of sameboy's curve.  Dropped into excel and fit this polynomial to it.
            color_lookup[i] =
                std::clamp((uint32_t)(-.0148 * (x * x * x) + .6243 * (x * x) + 2.9731 * x), (uint32_t)0, (uint32_t)255);
			//color_lookup[i] = std::clamp((uint32_t)(pow(((double)i / 31.0), gamma) * 255.0), (uint32_t)0, (uint32_t)255);
        }
    }
    int x = 1;
}

void c_gbppu::reset()
{
	line = 0;
	current_cycle = 0;
	SCX = 0;
	SCY = 0;
	STAT = 0;
	LCDC = 0x91;
	BGP = 0xFC;
	OBP0 = 0xFF;
	OBP1 = 0xFF;
	LY = 0;
	LYC = 0;
	WY = 0;
	WX = 0;
	DMA = 0;

	cpu_divider = 0;
	mode = 2;
	oam_index = 0;
	first_tile = 1;
	fetch_phase = 0;
	tile = 0;
	obj_p0 = 0;
	obj_p1 = 0;
	char_addr = 0;
	ybase = 0;
	p0_addr = 0;
	p1_addr = 0;
	render_buf_index = 0;
	current_pixel = 0;
	start_vblank = 0;
	fetch_x = 0;
	window_tile = 0;
	window_start_line = -1;

	dma_count = 0;
	sprite_count = 0;

	lcd_paused = 0;

	bg_latched = 0;

	stat_irq = 0;
	prev_stat_irq = 0;
	start_hblank = 0;

	memset(sprite_buffer, 0, sizeof(sprite_buffer));
	memset(vram, 0, 16384);
    memset(vram1, 0, 16384);

	memset(fb, 0xFF, 160 * 144 * sizeof(uint32_t));
	memset(fb_back, 0xFF, 160 * 144 * sizeof(uint32_t));

	SCX_latch = 0;

	cgb_vram_bank = 0;
    BCPS = 0;
    OBPS = 0;

	cgb_bg_attr = 0;
    KEY1 = 0;
    double_speed = 0;

	OPRI = 0;
}

void c_gbppu::eval_sprites(int y)
{
	int h = LCDC & 0x4 ? 16 : 8;
	if (h == 16) {
		int x = 1;
	}
	memset(sprite_buffer, 0, sizeof(sprite_buffer));
	s_sprite* s = (s_sprite*)oam;
	sprite_count = 0;
	for (int i = 0; i < 40; i++) {
		int sprite_y = s->y - 16;
		if (y >= sprite_y && y < (sprite_y + h)) {
			//sprite is in range, copy to sprite_buffer
			memcpy(&sprite_buffer[sprite_count], s, sizeof(s_sprite));
			if (h == 16) {
				sprite_buffer[sprite_count].tile &= ~0x1;
			}
			sprite_count++;
			if (sprite_count == 10)
				return;
		}
		s++;
	}
}

void c_gbppu::update_stat()
{
	if ((STAT & 0x44) == 0x44 ||
		(mode == 0 && (STAT & 0x8)) ||
		(mode == 1 && (STAT & 0x10)) ||
		(mode == 2 && (STAT & 0x20))) {
		stat_irq = 1;
		if (prev_stat_irq == 0) {
			gb->set_stat_irq(1);
		}
		prev_stat_irq = 1;
	}
	else {
		stat_irq = 0;
		if (prev_stat_irq == 1) {
			//gb->set_stat_irq(0);
		}
		prev_stat_irq = 0;
	}
}

void c_gbppu::set_ly(int line)
{
	if (LY == line)
		return;
	LY = line;
	if (LY == LYC) {
		STAT |= 0x4;
	}
	else {
		STAT &= ~0x4;
	}
	update_stat();
}

void c_gbppu::on_stop()
{
    if (KEY1 & 0x1) {
        double_speed ^= 0x1;
        KEY1 = double_speed ? 0x80 : 0;
    }
}

void c_gbppu::execute(int cycles)
{
	while (cycles-- > 0) {
		dots++;
		if (LCDC & 0x80) {

			if (current_cycle == 0) {
				set_ly(line);
			}
			else if (current_cycle == 1) {
				if (line == 153) {
					set_ly(0);
				}
			}

			switch (mode) {
			case 0:
				//mode 0 - hblank
				//85 - 208 dots
				//248 .. 455
				if (start_hblank) {
					start_hblank = 0;
					update_stat();
				}
				break;

			case 1:
				//mode 1 - vblank
				//10 lines
				if (start_vblank) {
					gb->set_vblank_irq(1);
					start_vblank = 0;
				}
				break;
			case 2:
				//mode 2 - scanning OAM
				//80 dots
				//0 .. 79
				if (current_cycle & 1) {
					//std::cout << "OAM lookup " << oam_index << std::endl;
					oam_index++;
				}
				if (current_cycle == 79) {
					lcd_paused = 0;
					eval_sprites(line);
					mode = 3;
					oam_index = 0;
					first_tile = 1;
					fetch_phase = 0;
					current_pixel = 0;
					f = fb_back + (line * 160);
					if (!(LCDC & 0x80)) {
						for (int j = 0; j < 160; j++) {
							*(f + j) = 0xFFFFFFFF;
						}
					}
					bg_latched = 0;
					for (auto& o : obj_fifo) {
						o = 0;
					}

					for (auto& b : bg_fifo) {
						b = -1;
					}

					bg_fifo_index = 0;
					obj_fifo_index = 0;
					obj_fifo_count = 0;
					obj_fetch_phase = 0;
					in_sprite_window = 0;
					sprites_here = 0;
					pixels_out = 0;
					done_drawing = 0;
					start_hblank = 0;
					in_window = 0;
					fetching_sprites = 0;
					SCX_latch = SCX;
					fetch_x = 0;

				}
				break;

			case 3:
				//mode 3 - rendering
				//168 - 291 dots
				//80 .. 247

				//if any sprites on current line, set fetch_sprite to number of sprites
				//and pause lcd

				if ((LCDC & 0x2) && !first_tile && current_pixel != 0 && sprites_here == 0)
				{
					for (int i = 0; i < sprite_count; i++) {
						if (sprite_buffer[i].x == current_pixel - (SCX_latch & 0x7)) {
							lcd_paused = 1;
							sprites_here++;
						}
					}
				}
				if ((LCDC & 0x20) && line >= WY) {
					int x = 1;
				}
				if ((LCDC & 0x20) && !in_window && !fetching_sprites && line >= WY) {
					if (current_pixel - 1 - (SCX_latch & 0x7) == WX) {
						in_window = 1;
						memset(bg_fifo, -1, sizeof(bg_fifo));
						fetch_phase = 0;
						bg_latched = 0;
						first_tile = 1;
						window_tile = 0;
						if (window_start_line == -1) {
							window_start_line = line;
						}
					}
				}

				switch (fetch_phase) {
				case 0: //sleep
					if (bg_fifo[bg_fifo_index] == -1) { //only proceed if bg_fifo is empty
						in_sprite_window = 0;
						if (bg_latched) {
							bg_latched = 0;
							//horizontal flip
                            int h_xor = cgb_bg_attr & 0x20 ? 7 : 0;
							for (int i = 0; i < 8; i++)
							{
								int shift = (7 - i) ^ h_xor;
								uint8_t p = ((bg_p0 >> shift) & 0x1) | (((bg_p1 >> shift) & 0x1) << 1);
                                p |= cgb_bg_attr & 0x80;
								bg_fifo[bg_fifo_index] = p;
                                if (gb->get_model() == GB_MODEL::CGB) {
                                    bg_fifo_pal[bg_fifo_index] = cgb_bg_attr & 0x7;
                                }
								bg_fifo_index = (bg_fifo_index + 1) & 0x7;
							}
						}
						fetch_phase++;
					}
					break;
				case 1: //fetch nt
					if (true || first_tile) {
						if (in_window) {
							ybase = window_start_line - WY;
							char_addr = 0x9800 | ((LCDC & 0x40) << 4) | ((ybase & 0xF8) << 2) | window_tile;
						}
						else {
							ybase = (line + SCY) & 0xFF;
							uint32_t xbase = ((SCX >> 3) + fetch_x) & 0x1F;
							char_addr = 0x9800 | ((LCDC & 0x8) << 7) | ((ybase & 0xF8) << 2) | xbase;
							//char_addr = 0x9800 | ((LCDC & 0x8) << 7) | ((ybase & 0xF8) << 2) | ((SCX & 0xF8) >> 3);
						}

					}
					tile = vram[char_addr - 0x8000];
                    if (gb->get_model() == GB_MODEL::CGB) {
                        cgb_bg_attr = vram1[char_addr - 0x8000];
                    }
					fetch_phase++;
					break;
				case 2: //sleep
					fetch_phase++;
					break;
				case 3: //fetch pt0
                {
					//vertical flip
                    uint32_t v_xor = cgb_bg_attr & 0x40 ? 7 : 0;
                    uint32_t y_adj = (ybase & 0x7) ^ v_xor;
                    if (LCDC & 0x10) {
                        p0_addr = (tile << 4) | (y_adj << 1);
                    }
                    else {
                        p0_addr = 0x800 + (((int8_t)tile + 128) << 4) + (y_adj << 1);
                    }
                    if (gb->get_model() == GB_MODEL::CGB) {
                        if (cgb_bg_attr & 0x8) {
                            bg_p0 = vram1[p0_addr];
                        }
                        else {
                            bg_p0 = vram[p0_addr];
                        }
                    }
                    else {
                        bg_p0 = vram[p0_addr];
                    }
                    fetch_phase++;
                }
					break;
				case 4: //sleep
					fetch_phase++;
					break;
				case 5: //fetch pt1
                    if (gb->get_model() == GB_MODEL::CGB) {
                        if (cgb_bg_attr & 0x8) {
                            bg_p1 = vram1[p0_addr + 1];
                        }
                        else {
                            bg_p1 = vram[p0_addr + 1];
                        }
                    }
                    else {
                        bg_p1 = vram[p0_addr + 1];
                    }
					bg_latched = 1;
					//increment lower 5 bits of char_addr
					if (!first_tile || in_window) {
						
						//char_addr = (char_addr & 0xFFE0) | ((char_addr + 1) & 0x1F);
						fetch_x++;
						window_tile++;
						in_sprite_window = 1;
						fetch_phase++;
					}
					else {
						fetch_phase = 0;
					}
					first_tile = 0;
					break;
				case 6:
					fetch_phase++;
					break;
				case 7:
					fetch_phase = 0;
					break;
				default:
					__assume(0);
				}


				if (sprites_here && in_sprite_window) {
					fetching_sprites = 1;
					int h = LCDC & 0x4 ? 16 : 8;
					switch (obj_fetch_phase) {
					case 0: //oam read
						for (int i = 0; i < sprite_count; i++) {
							if (sprite_buffer[i].x == current_pixel - (SCX_latch & 0x7)) {
								current_sprite = i;
								sprite_tile = sprite_buffer[current_sprite].tile;
								if (h == 16) {
									sprite_tile &= ~0x1;
								}
								sprite_y_offset = (line - sprite_buffer[current_sprite].y) & (h - 1);
								if (sprite_buffer[current_sprite].flags & 0x40) {
									sprite_y_offset = (h - 1) - sprite_y_offset;
								}
								break;
							}
						}
						obj_fetch_phase++;
						break;
					case 1: //oam read
						obj_fetch_phase++;
						break;
					case 2:
						obj_fetch_phase++;
						break;
					case 3:
					{
						sprite_addr = (sprite_tile << 4) | (sprite_y_offset << 1);
                        if (gb->get_model() == GB_MODEL::CGB) {
                            if (sprite_buffer[current_sprite].flags & 0x8) {
                                obj_p0 = vram1[sprite_addr];
                            }
                            else {
                                obj_p0 = vram[sprite_addr];
                            }
                        }
                        else {
                            obj_p0 = vram[sprite_addr];
                        }
						obj_fetch_phase++;
					}
						break;
					case 4:
						obj_fetch_phase++;
						break;
					case 5:
                        if (gb->get_model() == GB_MODEL::CGB) {
                            if (sprite_buffer[current_sprite].flags & 0x8) {
                                obj_p1 = vram1[sprite_addr + 1];
                            }
                            else {
                                obj_p1 = vram[sprite_addr + 1];
                            }
                        }
                        else {
                            obj_p1 = vram[sprite_addr + 1];
                        }
						{
							uint32_t obj_pri = sprite_buffer[current_sprite].flags & 0x80;
							uint32_t obj_pal = sprite_buffer[current_sprite].flags & 0x10;
							for (int i = 0; i < 8; i++)
							{
								int shift = sprite_buffer[current_sprite].flags & 0x20 ? i : 7 - i;
								uint8_t p = ((obj_p0 >> shift) & 0x1) | (((obj_p1 >> shift) & 0x1) << 1);

								uint32_t* o = &obj_fifo[(obj_fifo_index + i) & 0x7];
								//1... .... .... .... .... .ccc R..P ..pp
								//|                         |   |  |   +-pattern
								//|                         |   |  +-DMG Pal
								//|                         |   +-Priority
								//|                         +-CGB Pal
								//sprite here
                                uint32_t has_priority = gb->get_model() == GB_MODEL::CGB || !(*o & 0x80000000);
                                if (p != 0 && has_priority) {
                                    uint32_t cgb_pal = 0;
                                    if (gb->get_model() == GB_MODEL::CGB) {
                                        cgb_pal = sprite_buffer[current_sprite].flags & 0x7;
                                    }
                                    *o = (p | obj_pri | obj_pal | 0x80000000 | (cgb_pal << 8));
                                }
							}
						}
						obj_fetch_phase = 0;
						if (--sprites_here == 0) {
							fetching_sprites = 0;
							lcd_paused = 0;
						}
						break;
					default:
						__assume(0);
					}
				}


				if (!lcd_paused && bg_fifo[bg_fifo_index] != -1) {
					int p = bg_fifo[bg_fifo_index] & 0x3;
                    uint32_t cgb_bg_priority = bg_fifo[bg_fifo_index] & 0x80;
                    int pal = BGP;
                    uint32_t palx = 0;
                    if (gb->get_model() == GB_MODEL::CGB) {
						pal = bg_fifo_pal[bg_fifo_index];
						uint8_t pal1 = cgb_bg_pal[pal * 8 + (p * 2)];
						uint8_t pal2 = cgb_bg_pal[pal * 8 + (p * 2) + 1];
						palx = pal1 | pal2 << 8;
                    }
					if (!(LCDC & 0x1)) {
						p = 0;
					}
					bg_fifo[bg_fifo_index] = -1;
					bg_fifo_index = (bg_fifo_index + 1) & 0x7;
					uint8_t tile = 255;
					//if there is a sprite for this pixel
					if (obj_fifo[obj_fifo_index] & 0x80000000) {
						obj_fifo_count--;
						int s = obj_fifo[obj_fifo_index];
                        uint32_t sprite_priority = s & 0x80;
						obj_fifo[obj_fifo_index] = 0;

						//priority check
                        if (gb->get_model() == GB_MODEL::CGB) {
							if (p == 0 || (LCDC & 0x1) == 0 || (sprite_priority | cgb_bg_priority) == 0) {
									p = s | 0x80000000;
								}
                        }
                        else {
							if (sprite_priority == 0 || p == 0) {
								p = s | 0x80000000;
							}
                        }
					}
					obj_fifo_index = (obj_fifo_index + 1) & 0x7;

					if (p & 0x80000000) {
						//sprite
                        if (gb->get_model() == GB_MODEL::CGB) {
                            pal = (p >> 8) & 0x7;
                            p &= 0x3;
                            uint8_t pal1 = cgb_ob_pal[pal * 8 + (p * 2)];
                            uint8_t pal2 = cgb_ob_pal[pal * 8 + (p * 2) + 1];
                            palx = pal1 | pal2 << 8;
                        }
                        else {
                            pal = (p & 0x10) ? OBP1 : OBP0;
                        }
						p &= 0x3;
					}

					if (gb->get_model() == GB_MODEL::CGB) {
                        if (current_pixel >= (8 + (SCX_latch & 0x7))) {
                            uint8_t r = palx & 0x1F;
                            uint8_t g = (palx >> 5) & 0x1F;
                            uint8_t b = (palx >> 10) & 0x1F;

                            uint32_t col =
                                color_lookup[r] | (color_lookup[g] << 8) | (color_lookup[b] << 16) | (0xFF << 24);
                            *f++ = col;
                            pixels_out++;
                            if (pixels_out == 160) {
                                done_drawing = 1;
                            }
                        }
                    }
                    else {
                        if (current_pixel >= (8 + (SCX_latch & 0x7))) {
                            *f++ = shades[(pal >> (p * 2)) & 0x3];
                            pixels_out++;
                            if (pixels_out == 160) {
                            done_drawing = 1;
                            }
                        }
                    }

					current_pixel++;
				}

				//if (pixels_out == 160 || (!(LCDC & 0x80) && current_cycle == 247)) {
				if (current_cycle >= 251 && done_drawing) {
					mode = 0;
					start_hblank = 1;
				}
				break;
			}

			if (current_cycle++ == 455) { //end of line
				//end of line
				current_cycle = 0;
				line++;
				if (window_start_line != -1 && in_window) {
					window_start_line++;
				}
				if (line == 144) {
					//begin vblank
					start_vblank = 1;
					mode = 1;
					update_stat();
					std::swap(fb, fb_back);
				}
				else if (line == 154) {
					//end vblank
					//gb->set_vblank_irq(0);
					line = 0;
					mode = 2;
					update_stat();
					dots = 0;
					frame++;
					window_start_line = -1;
				}
				else if (line < 144) {
					mode = 2;
					update_stat();
				}
			}
		}
		cpu_divider = (cpu_divider + 1) & 0x3;
        if (cpu_divider == 0 || (double_speed && cpu_divider == 2)) {
            if (dma_count) {
                if (dma_count < 161) {
                int x = 160 - dma_count;
                oam[x] = gb->read_byte(DMA + x);
                }
                dma_count--;
            }
            gb->clock_timer();
            gb->cpu->execute(4);
        }
		if (cpu_divider == 0) {
            gb->apu->clock();
        }
	}
}

uint8_t c_gbppu::read_byte(uint16_t address)
{
	if (address < 0xA000) {
        if (gb->get_model() == GB_MODEL::CGB) {
            if (cgb_vram_bank & 0x1) {
                return vram1[address - 0x8000];
            }
            else {
                return vram[address - 0x8000];
            }
        }
		return vram[address - 0x8000];
	}
	else if (address >= 0xFE00 && address <= 0xFE9F) {
		return oam[address - 0xFE00];
	}
	else {
		switch (address) {
		case 0xFF40:
			return LCDC;
		case 0xFF41:
			if (mode == 0) {
				int x = 1;
			}
			return (STAT & ~0x3) | (mode & 0x3);
		case 0xFF42:
			return SCY;
		case 0xFF43:
			return SCX;
		case 0xFF44:
			return LY;
		case 0xFF45:
			return LYC;
		case 0xFF46:
			return DMA;
		case 0xFF47:
			return BGP;
		case 0xFF48:
			return OBP0;
		case 0xFF49:
			return OBP1;
		case 0xFF4A:
			return WY;
		case 0xFF4B:
			return WX;
        case 0xFF4D:
            return KEY1;
        case 0xFF4F:
            return cgb_vram_bank;
        case 0xFF68:
            return BCPS;
        case 0xFF69:
            return BCPS;
        case 0xFF6A:
            return OBPS;
        case 0xFF6B:
            return OBPS;
        case 0xFF6C:
            return OPRI;
		default:
			//printf("unhandled read from ppu\n");
			//exit(0);
            int x = 1;
			break;
		}
	}
	return 0;
}

void c_gbppu::write_byte(uint16_t address, uint8_t data)
{
	if (address < 0xA000) {
        if (gb->get_model() == GB_MODEL::CGB) {
			if (cgb_vram_bank & 0x1) {
				vram1[address - 0x8000] = data;
			}
            else {
                vram[address - 0x8000] = data;
            }
        }
        else {
            vram[address - 0x8000] = data;
        }
	}
	else if (address >= 0xFE00 && address <= 0xFE9F) {
		oam[address - 0xFE00] = data;
	}
	else {
		switch (address) {
		case 0xFF40:
			LCDC = data;
			if (LCDC & 0x80 && !(data & 0x80)) {
				LY = 0;
				line = 0;
				current_cycle = 0;
				memset(gb, 0xFF, 160*144 * sizeof(uint32_t));
			}
			break;
		case 0xFF41:
			STAT = data;
			break;
		case 0xFF42:
			SCY = data;
			break;
		case 0xFF43:
			SCX = data;
			break;
		case 0xFF44:
			LY = data;
			break;
		case 0xFF45:
			LYC = data;
			if (LY == LYC) {
				STAT |= 0x4;
			}
			else {
				STAT &= ~0x4;
			}
			update_stat();
			break;
		case 0xFF46:
			DMA = data << 8;
			dma_count = 160;
			break;
		case 0xFF47:
			BGP = data;
			break;
		case 0xFF48:
			OBP0 = data;
			break;
		case 0xFF49:
			OBP1 = data;
			break;
		case 0xFF4A:
			WY = data;
			break;
		case 0xFF4B:
			WX = data;
			break;
        case 0xFF4D:
            KEY1 = (KEY1 & 0xFE) | data & 1;
            break;
        case 0xFF4F:
            cgb_vram_bank = data & 0x1;
            break;
        case 0xFF68:
            BCPS = data;
            break;
        case 0xFF69: {
            int i = BCPS & 0x3F;
            cgb_bg_pal[i] = data;
            if (BCPS & 0x80) {
                i += 1;
                BCPS = 0x80 | (i & 0x3F);
            }
        } break;
        case 0xFF6A:
            OBPS = data;
            break;
        case 0xFF6B: {
            int i = OBPS & 0x3F;
            cgb_ob_pal[i] = data;
            if (OBPS & 0x80) {
                i += 1;
                OBPS = 0x80 | (i & 0x3F);
            }
        } break;
        case 0xFF6C:
            OPRI = data & 1;
            break;

		default:
			//printf("unhandled write to ppu\n");
			//exit(0);
            {
                char buf[64];
                sprintf(buf, "Unhandled PPU write 0x%02X -> 0x%04X\n", data, address);
                OutputDebugString(buf);
            }
            int x = 1;
			break;
		}
	}
	return;
}