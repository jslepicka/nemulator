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
#include "apu.h"
#include "memory.h"
#include "mapper.h"
#include "apu2.h"

#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

long c_ppu::lookup_tables_built = 0;
int c_ppu::attr_shift_table[0x400];
int c_ppu::attr_loc[0x400];
int c_ppu::mortonOdd[256];
int c_ppu::mortonEven[256];
__int32 c_ppu::morton_odd_32[256];
__int32 c_ppu::morton_even_32[256];
__int64 c_ppu::morton_odd_64[256];
__int64 c_ppu::morton_even_64[256];

c_ppu::c_ppu(void)
{
	pSpriteMemory = 0;
	mapper = 0;
	build_lookup_tables();
	limit_sprites = false; //don't change this on reset
}

void c_ppu::build_lookup_tables()
{
	if (InterlockedCompareExchange(&lookup_tables_built, 1, 0) == 0)
	{
		for (int i = 0; i < 0x400; ++i)
		{
			attr_shift_table[i] = ((i >> 4) & 0x04) | (i & 0x02);
			attr_loc[i] = ((i >> 4) & 0x38) | ((i >> 2) & 0x07);
		}

		for (int i = 0; i < 256; i++)
		{
			int odd_result = 0;
			int even_result = 0;
			int result = 0;
			__int32 result_32 = 0;
			__int64 result_64 = 0;
			int temp = i;
			for (int b = 0; b < 8; b++)
			{
				result |= ((i & (1 << b)) << ((b * 2) - b));
				result_32 <<= 4;
				result_32 |= (temp & 1);
				result_64 <<= 8;
				result_64 |= (temp & 1);
				temp >>= 1;
			}
			mortonOdd[i] = result;
			mortonEven[i] = result << 1;
			morton_odd_32[i] = result_32;
			morton_even_32[i] = result_32 << 1;
			morton_odd_64[i] = result_64;
			morton_even_64[i] = result_64 << 1;
		}
	}
}

c_ppu::~c_ppu(void)
{
	if (pSpriteMemory)
		delete[] pSpriteMemory;
}

__forceinline int c_ppu::InterleaveBits(unsigned char odd, unsigned char even)
{
	return mortonOdd[odd] | mortonEven[even];
}

__forceinline __int32 c_ppu::interleave_bits_32(unsigned char odd, unsigned char even)
{
	return morton_odd_32[odd] | morton_even_32[even];
}

__forceinline __int64 c_ppu::interleave_bits_64(unsigned char odd, unsigned char even)
{
	return morton_odd_64[odd] | morton_even_64[even];
}
unsigned char c_ppu::ReadByte(int address)
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
		//if (current_scanline == 0 && current_cycle == 0)
		//	temp &= ~0x80;
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
			if (current_cycle < 64)
				return_value = 0xFF;
			else if (current_cycle < 256)
				return_value = 0x00; //this isn't correct, but I don't think anything relies on it
			else if (current_cycle < 320)
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

void c_ppu::Reset(void)
{
	rendering_state = 0;
	odd_frame = 0;
	intensity = 0;
	palette_mask = -1;
	tick = 0;
	current_cycle = 0;
	nmi_pending = false;
	current_scanline = 0;
	rendering = 0;
	short_scanline = 0;
	//render_address = 0;
	pattern_address = attribute_address = 0;
	on_screen = 0;
	control1 = (unsigned char *)&ppuControl1;
	control2 = (unsigned char *)&ppuControl2;
	status = (unsigned char *)&ppuStatus;
	if (!pSpriteMemory)
		pSpriteMemory = new unsigned char[256];
	memset(pSpriteMemory, 0, 256);
	pFrameBuffer = (int *)&frameBuffer;
	memset(pFrameBuffer, 0, 256 * 256 * sizeof(int));
	//limitSprites = false;
	memset(imagePalette, 0x3F, 32);
	readValue = 0;
	*control1 = 0;
	*control2 = 0;
	*status = 0;
	hi = false;
	spriteMemAddress = 0;
	linenumber = 0;
	addressIncrement = 1;
	//background must be disabled for crystal mines intro to work
	//not sure why it was previously set to true
	//ppuControl2.backgroundSwitch = true;
	vramAddress = vramAddressLatch = fineX = 0;
	bgPatternTableAddress = 0;
	drawingBg = 0;
	sprite0_hit_location = -1;
	executed_cycles = 3;
	//limit_sprites = false;
	four_screen = false;
	mirroring_mode = 0;
	pattern1 = pattern2 = 0;

	pattern_address = attribute_address = 0;

	memset(index_buffer, 0, sizeof(index_buffer));
	memset(image_palette, 0x3F, 32);
	memset(sprite_buffer, 0, sizeof(sprite_buffer));

	sprites_visible = 0;

}

//int c_ppu::get_mirroring_mode()
//{
//	return mirroring_mode;
//}

void c_ppu::StartFrame(void)
{
	if (ppuControl2.backgroundSwitch || ppuControl2.spritesVisible)
		vramAddress = vramAddressLatch;
}

void c_ppu::run_ppu(int cycles)
{
	for (int cycle = 0; cycle < cycles; ++cycle)
	{
		switch (current_cycle)
		{
		case 0:
			switch (current_scanline)
			{
			case 0:
				//start vblank
				ppuStatus.vBlank = true;
				if (ppuControl1.vBlankNmi)
				{
					nmi_pending = true;
					cpu->execute_nmi();
				}
				break;
			case 20:
				//begin renderring
				rendering = DrawingEnabled();
				ppuStatus.spriteCount = false;
				ppuStatus.vBlank = false;
				ppuStatus.hitFlag = false;
				p_frame = pFrameBuffer;
				//end vblank
				break;
			default:
				if (current_scanline >= 21)
				{
					//begin on-screen
					on_screen = 1;
				}
				break;
			}
			break;
		case 256:
			on_screen = 0;
			break;
		case 304:
			if (current_scanline == 20 && DrawingEnabled())
				vramAddress = vramAddressLatch;
			break;
		case 340:
			if (current_scanline == 260)
			{
				rendering = on_screen = 0;
			}
			break;
		}


		if (rendering)
		{
			switch (current_cycle)
			{
			case   0:	case   8:	case  16:	case  24:
			case  32:	case  40:	case  48:	case  56:
			case  64:	case  72:	case  80:	case  88:
			case  96:	case 104:	case 112:	case 120:
			case 128:	case 136:	case 144:	case 152:
			case 160:	case 168:	case 176:	case 184:
			case 192:	case 200:	case 208:	case 216:
			case 224:	case 232:	case 240:	case 248:
			case 320:	case 328:
				drawingBg = true;
				tile = mapper->ppu_read(0x2000 | (vramAddress & 0xFFF));
				break;
			case   1:	case   9:	case  17:	case  25:
			case  33:	case  41:	case  49:	case  57:
			case  65:	case  73:	case  81:	case  89:
			case  97:	case 105:	case 113:	case 121:
			case 129:	case 137:	case 145:	case 153:
			case 161:	case 169:	case 177:	case 185:
			case 193:	case 201:	case 209:	case 217:
			case 225:	case 233:	case 241:	case 249:
			case 321:	case 329:
				pattern_address = (tile << 4) + ((vramAddress & 0x7000) >> 12) + (ppuControl1.screenPatternTableAddress * 0x1000);
				break;
			case   2:	case  10:	case  18:	case  26:
			case  34:	case  42:	case  50:	case  58:
			case  66:	case  74:	case  82:	case  90:
			case  98:	case 106:	case 114:	case 122:
			case 130:	case 138:	case 146:	case 154:
			case 162:	case 170:	case 178:	case 186:
			case 194:	case 202:	case 210:	case 218:
			case 226:	case 234:	case 242:	case 250:
			case 322:	case 330:
				attribute_address = 0x23C0 |
					(vramAddress & 0xC00) |
					(attr_loc[vramAddress & 0x3ff]);
				attribute_shift = attr_shift_table[vramAddress & 0x3FF];
				break;
			case   3:	case  11:	case  19:	case  27:
			case  35:	case  43:	case  51:	case  59:
			case  67:	case  75:	case  83:	case  91:
			case  99:	case 107:	case 115:	case 123:
			case 131:	case 139:	case 147:	case 155:
			case 163:	case 171:	case 179:	case 187:
			case 195:	case 203:	case 211:	case 219:
			case 227:	case 235:	case 243:   case 251:
			case 323:   case 331:
				attribute = mapper->ppu_read(attribute_address);
				attribute >>= attribute_shift;
				attribute &= 0x03;
				//attribute <<= 2;

				if ((vramAddress & 0x1F) == 0x1F)
					vramAddress ^= 0x41F;
				else
					vramAddress++;
				if (current_cycle == 251)
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
				break;
			case   4:	case  12:	case  20:	case  28:
			case  36:	case  44:	case  52:	case  60:
			case  68:	case  76:	case  84:	case  92:
			case 100:	case 108:	case 116:	case 124:
			case 132:	case 140:	case 148:	case 156:
			case 164:	case 172:	case 180:	case 188:
			case 196:	case 204:	case 212:	case 220:
			case 228:	case 236:	case 244:	case 252:
			case 324:	case 332:
				break;
			case   5:	case  13:	case  21:	case  29:
			case  37:	case  45:	case  53:	case  61:
			case  69:	case  77:	case  85:	case  93:
			case 101:	case 109:	case 117:	case 125:
			case 133:	case 141:	case 149:	case 157:
			case 165:	case 173:	case 181:	case 189:
			case 197:	case 205:	case 213:	case 221:
			case 229:	case 237:	case 245:	case 253:
			case 325:   case 333:
				drawingBg = true;
				pattern1 = mapper->ppu_read(pattern_address);
				break;
			case   6:	case  14:	case  22:	case  30:
			case  38:	case  46:	case  54:	case  62:
			case  70:	case  78:	case  86:	case  94:
			case 102:	case 110:	case 118:	case 126:
			case 134:	case 142:	case 150:	case 158:
			case 166:	case 174:	case 182:	case 190:
			case 198:	case 206:	case 214:	case 222:
			case 230:	case 238:	case 246:	case 254:
			case 326:	case 334:
				break;
			case   7:	case  15:	case  23:	case  31:
			case  39:	case  47:	case  55:	case  63:
			case  71:	case  79:	case  87:	case  95:
			case 103:	case 111:	case 119:	case 127:
			case 135:	case 143:	case 151:	case 159:
			case 167:	case 175:	case 183:	case 191:
			case 199:	case 207:	case 215:	case 223:
			case 231:	case 239:	case 247:	case 255:
			case 327:   case 335:
			{
				pattern2 = mapper->ppu_read(pattern_address + 8);
				unsigned int l = current_cycle + 9;
				if (l > 335)
					l -= 336;

#ifdef WIN64
				__int64 *ib64 = (__int64*)&index_buffer[l];
				__int64 c = interleave_bits_64(pattern1, pattern2) | (attribute * 0x0404040404040404ULL);
				*ib64 = c;
#else
				attribute *= 0x04040404;
				int *ib = (int*)&index_buffer[l];
				int *p1 = (int*)morton_odd_64 + pattern1 * 2;
				int *p2 = (int*)morton_even_64 + pattern2 * 2;
				for (int i = 0; i < 2; i++)
					*ib++ = *p1++ | *p2++ | attribute;
#endif
				drawingBg = false;
			}
				mapper->mmc5_inc_tile();
				break;
			case 256:	case 264:	case 272:   case 280:
			case 288:	case 296:	case 304:	case 312:
				//tile = mapper->ppu_read(0x2000 | (vramAddress & 0xFFF));
				//dummy reads
				mapper->ppu_read(0x2000);
				break;
			case 257:
				vramAddress &= ~0x41F;
				vramAddress |= (vramAddressLatch & 0x41F);
				break;
			case 258:	case 266:	case 274:	case 282:
			case 290:	case 298:	case 306:	case 314:
				mapper->ppu_read(0x2000);
				break;
			case 265:	case 273:	case 281:	case 289:
			case 297:	case 305:	case 313:
				break;
			case 259:	case 267:	case 275:	case 283:
			case 291:	case 299:	case 307:	case 315:
				break;
			case 260:	case 268:	case 276:	case 284:
			case 292:	case 300:	case 308:	case 316:

				if (ppuControl1.spriteSize)
				{
					int sprite_tile = sprite_buffer[(((current_cycle - 260) / 8) * 4) + 1];
					mapper->ppu_read((sprite_tile & 0x1) * 0x1000);
				}
				else
				{
					mapper->ppu_read(ppuControl1.spritePatternTableAddress * 0x1000);
				}
				break;
			case 261:	case 269:	case 277:	case 285:
			case 293:	case 301:	case 309:	case 317:
				break;

			case 262:	case 270:	case 278:	case 286:
			case 294:	case 302:	case 310:	case 318:
				if (ppuControl1.spriteSize)
				{
					int sprite_tile = sprite_buffer[(((current_cycle - 260) / 8) * 4) + 1];
					mapper->ppu_read((sprite_tile & 0x1) * 0x1000);
				}
				else
				{
					mapper->ppu_read(ppuControl1.spritePatternTableAddress * 0x1000);
				}
				break;
			case 263:	case 271:	case 279:	case 287:
			case 295:	case 303:	case 311:	case 319:
				break;
			case 336:	case 337:	case 338:
				break;
			case 339:
				if (odd_frame && current_scanline == 20)
				{
					current_cycle++;
					cycle++;
				}
				break;
			case 340:
				break;
			}
			if (on_screen)
			{
				int pixel = 0;
				int bg_index = 0;

				if (ppuControl2.backgroundSwitch && ((current_cycle >= 8) || ppuControl2.backgroundClipping))
					bg_index = index_buffer[current_cycle + fineX];

				if (bg_index & 0x03)
				{
					pixel = (image_palette[bg_index] & palette_mask) | intensity;
				}
				else
				{
					pixel = (image_palette[0] & palette_mask) | intensity;
				}

				if (sprite_count && ppuControl2.spritesVisible && ((current_cycle >= 8) || ppuControl2.spriteClipping))
				{
					int max_sprites = limit_sprites ? 8 : 64;
					for (int i = 0; i < sprite_count && i < max_sprites; i++)
					{
						int sprite_x = sprite_buffer[(i * 4) + 3];

						if ((unsigned int)(current_cycle - sprite_x) < 8)
						{
							int priority = sprite_buffer[(i * 4) + 2] & 0x20;
							int sprite_color = sprite_index_buffer[i * 8 + (current_cycle - sprite_x)];
							if (sprite_color & 0x03)
							{
								if (i == sprite0_index &&
									ppuControl2.backgroundSwitch &&
									(bg_index & 0x03) != 0 &&
									current_cycle < 255)
									ppuStatus.hitFlag = true;

								if (!(priority && (bg_index & 0x03)))
									pixel = (image_palette[16 + sprite_color] & palette_mask) | intensity;
								break;
							}
						}
					}
				}
				*p_frame++ = pixel;
			}
		}
		else if (on_screen)
		{
			int pixel = 0;
			if ((vramAddress & 0x3F00) == 0x3F00)
			{
				pixel = (image_palette[vramAddress & 0x1F] & palette_mask) | intensity;
			}
			else
			{
				pixel = (image_palette[0] & palette_mask) | intensity;
			}
			*p_frame++ = pixel;
		}

		mapper->clock(1);
		if (--executed_cycles == 0)
		{
			//cpu->execute_cycles(3);
			//mapper->clock(3);
			cpu->execute3();
			apu2->clock_once();
			executed_cycles = 3;
		}
		++current_cycle;
		if (current_cycle == 341)
		{
			current_scanline++;
			if (current_scanline == 262)
				current_scanline = 0;
			current_cycle = 0;
			odd_frame ^= 1;
		}
	}
}

void c_ppu::DrawScanline(void)
{
	if (linenumber < 240)
	{
		drawingBg = 1;
		DrawBackgroundLine();
		drawingBg = 0;
		DrawSpriteLine();
	}

	if (linenumber == 239) //do it all over again
	{
		linenumber = 0;
	}
	else
		linenumber++;
}

void c_ppu::StartScanline(void)
{
	if (!DrawingEnabled())
		return;
	// Update vramAddress with H and HT from vramAddressLatch
	vramAddress &= 0xFBE0;
	vramAddress |= (vramAddressLatch & 0x041F);
}

void c_ppu::reset_hscroll()
{
	//at cycle 257
	if (!DrawingEnabled())
		return;
	// Update vramAddress with H and HT from vramAddressLatch
	vramAddress &= 0xFBE0;
	vramAddress |= (vramAddressLatch & 0x041F);
}

void c_ppu::reset_vscroll()
{
	//at cycle 251
	if (!DrawingEnabled())
		return;
	// if FV = 7, prepare to increment VT
	if ((vramAddress & 0x7000) == 0x7000)
	{
		// Set FV to 0
		vramAddress &= 0x8FFF;

		// Check VT
		switch (vramAddress & 0x03E0)
		{
		case 0x03A0: //29
			vramAddress ^= 0x0800;
			//fall through
		case 0x03E0: //31
			// VT = 0;
			vramAddress &= 0xFC1F;
			break;
		default:
			vramAddress += 0x0020;
		}
	}
	else
	{
		//increment FV
		vramAddress += 0x1000;
	}
}

void c_ppu::EndScanline(void)
{
	if (!DrawingEnabled())
		return;
	// if FV = 7, prepare to increment VT
	if ((vramAddress & 0x7000) == 0x7000)
	{
		// Set FV to 0
		vramAddress &= 0x8FFF;

		// Check VT
		switch (vramAddress & 0x03E0)
		{
		case 0x03A0: //29
			vramAddress ^= 0x0800;
			//fall through
		case 0x03E0: //31
			// VT = 0;
			vramAddress &= 0xFC1F;
			break;
		default:
			vramAddress += 0x0020;
		}
	}
	else
	{
		//increment FV
		vramAddress += 0x1000;
	}
}

bool c_ppu::DrawingEnabled(void)
{
	if (ppuControl2.backgroundSwitch || ppuControl2.spritesVisible)
		return true;
	else
		return false;
}

int c_ppu::GetSpriteSize()
{
	return (int)ppuControl1.spriteSize;
}

int c_ppu::eval_sprites()
{
	if (!rendering)
		return -1;
	mapper->in_sprite_eval = 1;
	int max_sprites = limit_sprites ? 8 : 64;
	int sprite0_x = -1;
	sprite0_index = -1;
	sprite_count = 0;
	memset(sprite_buffer, 0xFF, sizeof(sprite_buffer));
	for (int i = 0; i < 64/* && sprite_count < max_sprites*/; i++)
	{
		int sprite_offset = i * 4;
		int y = *(pSpriteMemory + sprite_offset) + 1;
		int sprite_height = 8 << (int)ppuControl1.spriteSize;
		if (((current_scanline - 21) >= y) && ((current_scanline - 21) < (y + sprite_height)))
		{
			if (i == 0)
			{
				sprite0_x = *(pSpriteMemory + sprite_offset + 3);
				sprite0_index = sprite_count;
			}
			memcpy(&sprite_buffer[sprite_count * 4], pSpriteMemory + sprite_offset, 4);


			int sprite_tile = *(pSpriteMemory + sprite_offset + 1);
			int sprite_y = (current_scanline - 21) - y;
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


			//int pattern = InterleaveBits(mapper->ppu_read(sprite_address), mapper->ppu_read(sprite_address + 8));
			////int priority = sprite_attribute & 0x20;
			//int attr = (sprite_attribute & 0x03) << 2;
			//for (int p = 0; p < 8; p++)
			//{
			//	int color = (pattern >> (sprite_attribute & 0x40 ? p * 2 : 14 - p * 2)) & 0x03;
			//	color |= attr;
			//	sprite_index_buffer[(sprite_count * 8) + p] = color;
			//}


			int attr = ((sprite_attribute & 0x03) << 2) * 0x01010101;
			int *p1 = (int*)morton_odd_64 + mapper->ppu_read(sprite_address) * 2;
			int *p2 = (int*)morton_even_64 + mapper->ppu_read(sprite_address + 8) * 2;

			int p[2] = { *p1++ | *p2++ | attr, *p1 | *p2 | attr };

			if (sprite_attribute & 0x40)
			{
				unsigned char *sb = &sprite_index_buffer[sprite_count * 8];
				unsigned char *pc = (unsigned char*)p + 7;
				for (int j = 0; j < 8; j++)
					*sb++ = *pc--;
			}
			else
			{
				int *sb = (int*)&sprite_index_buffer[sprite_count * 8];
				for (int j = 0; j < 2; j++)
					*sb++ = p[j];

			}

			sprite_count++;
			if (sprite_count == 9)
				ppuStatus.spriteCount = true;
		}
	}
	mapper->in_sprite_eval = 0;
	if (current_scanline == 21)
		sprite_count = 0;
	return sprite0_x;
}

void c_ppu::DrawSpriteLine(void)
{
	if (!ppuControl2.spritesVisible)
		return;
	int numSprites = 0;
	ppuStatus.spriteCount = false;
	int *pF = pFrameBuffer + (linenumber * 256);
	for (int spriteNumber = 0; spriteNumber < 64; spriteNumber++)
	{

		int spriteOffset = spriteNumber * 4;
		int spriteHeight = 8 << (int)ppuControl1.spriteSize;

		int yCoord = *(pSpriteMemory + spriteOffset) + 1;
		int xCoord = *(pSpriteMemory + spriteOffset + 3);

		if ((linenumber >= yCoord) && (linenumber < yCoord + spriteHeight))
		{
			numSprites++;
			if (numSprites > 8 && ppuStatus.spriteCount != true)
			{
				ppuStatus.spriteCount = true;
				if (limit_sprites)
					return;
			}
			int TileIndex = *(pSpriteMemory + spriteOffset + 1);

			int y = linenumber - yCoord;
			int spriteAttribute = *(pSpriteMemory + spriteOffset + 2);

			//Vertical flip?
			if (spriteAttribute & 0x80)
			{
				y = (spriteHeight - 1) - y;
			}

			int spriteAddress;

			if (spriteHeight == 16)
			{
				spriteAddress = ((TileIndex & 0xFE) * 16) + (y & 0x07) + ((TileIndex & 0x01) * 0x1000);
				if (y > 7) spriteAddress += 16;
			}
			else
				spriteAddress = (TileIndex * 16) + (y & 0x07) + (ppuControl1.spritePatternTableAddress * 0x1000);

			int pattern = InterleaveBits(mapper->ppu_read(spriteAddress), mapper->ppu_read(spriteAddress + 8));

			int priority = spriteAttribute & 0x20;
			int *pF2 = pF + xCoord;
			int attr = (spriteAttribute & 0x03) << 2;
			for (int p = 0; p < 8; p++)
			{
				if (xCoord + p > 255)
					break;

				if (xCoord + p < 8 && ppuControl2.spriteClipping == 0)
				{
					pF2++;
					continue;
				}

				int color = (pattern >> (spriteAttribute & 0x40 ? p * 2 : 14 - p * 2)) & 0x03;

				if (color)
				{
					if (spriteNumber == 0)
					{
						if (!ppuStatus.hitFlag && sprite0_hit_location == -1 && ppuControl2.backgroundSwitch && (xCoord + p != 255) && !(*pF2 & FB_TRANSPARENTBG))
						{
							//ppuStatus.hitFlag = true;
							sprite0_hit_location = (xCoord + p);
						}
					}

					color |= attr;

					if (priority)
					{
						//behind
						if (!(*pF2 & FB_SPRITEDRAWN))
						{
							//if transparent background draw sprite
							if (*pF2 & FB_TRANSPARENTBG)
								*pF2 = ((image_palette[color + 16] & palette_mask) | intensity) | FB_SPRITEDRAWN;
							else
								*pF2 |= FB_SPRITEDRAWN;
						}
					}
					else
					{
						if (!(*pF2 & FB_SPRITEDRAWN))
						{
							*pF2 = ((image_palette[color + 16] & palette_mask) | intensity) | FB_SPRITEDRAWN;
						}
					}
				}
				pF2++;
			}
		}
	}
}

void c_ppu::DrawBackgroundLine(void)
{
	int *pF = pFrameBuffer + (linenumber * 256);

	if (!ppuControl2.backgroundSwitch)
	{
		for (int x = 0; x < 256; x++)
			*pF++ = ((image_palette[0] & palette_mask) | intensity);
		return;
	}

	int htile = (vramAddress & 0x001F);
	int vtile = (vramAddress & 0x03E0) >> 5;
	int nameAddress = 0x2000 + (vramAddress & 0x0FFF);
	int attAddress = 0x23C0 + (vramAddress & 0x0C00) + ((vtile & 0xFFFC) << 1) + (htile >> 2);
	//int attValue = mapper->ppu_read(attAddress) >> (vtile & 0x02 ? 4 : 0);
	int patternBase = bgPatternTableAddress + ((vramAddress & 0x7000) >> 12);

	unsigned int x = -fineX;

	for (int i = 0; i < 33; i++)
	{
		int patternAddress = patternBase + (mapper->ppu_read(nameAddress) << 4);
		int attValue = mapper->ppu_read(attAddress) >> (vtile & 0x02 ? 4 : 0);
		int attColor = ((attValue >> (htile & 0x02 ? 2 : 0)) & 0x03) << 2;

		unsigned char pattern1 = mapper->ppu_read(patternAddress);
		unsigned char pattern2 = mapper->ppu_read(patternAddress + 8);
		int pattern = InterleaveBits(pattern1, pattern2);

		for (int q = 14; q >= 0; q -= 2)
		{
			if (x < 256)
			{
				if (x < 8 && ppuControl2.backgroundClipping == 0)
				{

					*pF = ((image_palette[0] & palette_mask) | intensity) | FB_CLIPPED;
				}
				else
				{

					int color = (pattern >> q) & 0x03;

					if (color)
					{
						color |= attColor;
						*pF = ((image_palette[color] & palette_mask) | intensity);
					}
					else
						*pF = ((image_palette[0] & palette_mask) | intensity) | FB_TRANSPARENTBG;


				}
				pF++;
			}
			x++;
		}

		htile++;
		mapper->mmc5_inc_tile();
		nameAddress++;

		if ((htile & 0x03) == 0)
		{
			//4 tile boundary
			if ((htile & 0x1F) == 0)
			{
				//32 tile boundary
				nameAddress ^= 0x0400;
				attAddress ^= 0x0400;
				nameAddress -= 0x0020;
				attAddress -= 0x0008;
			}
			attAddress++;
			//attValue = mapper->ppu_read(attAddress) >> (vtile & 0x02 ? 4 : 0);
		}
	}
}

bool c_ppu::DoA12()
{
	if (DrawingEnabled() &&
		((!ppuControl1.spriteSize && (ppuControl1.screenPatternTableAddress != ppuControl1.spritePatternTableAddress)) ||
		(ppuControl1.spriteSize && !ppuControl1.screenPatternTableAddress)))
		return true;
	else
		return false;
}

void c_ppu::WriteByte(int address, unsigned char value)
{
	switch (address)
	{
	case 0x2000:    //PPU Control Register 1
	{
		if (((*control1 ^ value) & 0x80 & value) && ppuStatus.vBlank)
			cpu->execute_nmi();

		*control1 = value;
		if (ppuControl1.verticalWrite)
		{
			addressIncrement = 32;
		}
		else
		{
			addressIncrement = 1;
		}
		bgPatternTableAddress = ((value & 0x10) != 0) ? 0x1000 : 0x0000;
		vramAddressLatch &= 0x73FF;
		vramAddressLatch |= ((value & 3) << 10);
		break;
	}
	case 0x2001:    //PPU Control Register 2
	{
		*control2 = value;
		sprites_visible = ppuControl2.spritesVisible;
		if ((value & 0x18) && (current_scanline > 19 && current_scanline < 261)) //rendering
			rendering = true;
		else
			rendering = false;
		if (value & 0x01)
		{
			palette_mask = 0x30;
		}
		else
			palette_mask = -1;
		intensity = (value & 0xE0) << 1;
		break;
	}
	case 0x2002:    //PPU Status Register
	{
		//*status = value;
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
				vramAddress = vramAddressLatch;
				if (!rendering)
				{
				mapper->ppu_read(vramAddress);
			}
		}
		hi = !hi;
		break;
	}
	case 0x2007:    //PPU Memory Data
	{
		if ((vramAddress & 0x3FFF) >= 0x3F00)
		{
			int pal_address = vramAddress & 0x1F;
			value &= 0x3F;
			image_palette[pal_address] = value;
			if (!(pal_address & 0x03))
			{
				image_palette[pal_address ^ 0x10] = value;
			}
			if (!rendering)
				mapper->ppu_read(vramAddress);
		}
		else
		{
			if (!rendering)
				mapper->ppu_write(vramAddress, value);
		}
		update_vram_address();
	}
	}
}

int c_ppu::BeginVBlank(void)
{
	ppuStatus.vBlank = true;
	return ppuControl1.vBlankNmi;
}

void c_ppu::EndVBlank(void)
{
	ppuStatus.vBlank = false;
	ppuStatus.hitFlag = false;
	sprite0_hit_location = -1;
	ppuStatus.spriteCount = false;
}

void c_ppu::update_vram_address()
{
	if (rendering)
	{
		if (true || addressIncrement == 32)
		{
			if ((vramAddress & 0x7000) == 0x7000)
			{
				// Set FV to 0
				vramAddress &= 0x0FFF;

				// Check VT
				switch (vramAddress & 0x03E0)
				{
				case 0x03A0:
					vramAddress ^= 0x0800;
				case 0x03E0:
					vramAddress &= 0xFC1F;
					break;
				default:
					vramAddress += 0x0020;
				}
			}
			else
				//increment FV
				vramAddress += 0x1000;
		}
		else
			vramAddress = (vramAddress + addressIncrement) & 0x7FFF;
	}
	else
	{
		//update bus when not rendering
		vramAddress = (vramAddress + addressIncrement) & 0x7FFF;
		mapper->ppu_read(vramAddress);
	}
}