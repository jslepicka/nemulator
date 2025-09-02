module;
#include <Windows.h>
#include <cassert>
#include <immintrin.h>
#include <intrin.h>
module genesis:vdp;

#define USE_BMI2

namespace genesis
{

c_vdp::c_vdp(uint8_t *ipl, read_word_t read_word_68k, mode_switch_callback_t mode_switch_callback, uint32_t *stalled)
{
    this->stalled = stalled;
    this->ipl = ipl;
    this->read_word_68k = read_word_68k;
    this->mode_switch_callback = mode_switch_callback;

    for (int i = 0; i < 512; i++) {
        uint32_t color = 0xFF000000;
        color |= ((((i >> 0) & 7) * 32)) << 0;
        color |= ((((i >> 3) & 7) * 32)) << 8;
        color |= ((((i >> 6) & 7) * 32)) << 16;
        rgb[i] = color;
    }
    
    plane_ptrs[0] = sprite_out;
    plane_ptrs[1] = win_out;
    plane_ptrs[2] = a_out;
    plane_ptrs[3] = b_out;
}

c_vdp::~c_vdp()
{
}

void c_vdp::reset()
{
    //status.value = 0x3400;
    status.value = 0;
    line = 0;
    std::memset(reg, 0, sizeof(reg));
    address_reg = 0;
    address_write = 0;
    memset(vram, -1, sizeof(vram));
    memset(cram, 0, sizeof(cram));
    memset(vsram, 0, sizeof(vsram));
    memset(frame_buffer, 0, sizeof(frame_buffer));
    line = 0;
    freeze_cpu = 0;
    pending_fill = 0;
    asserting_hblank = 0;
    asserting_vblank = 0;
    update_ipl();
    hint_counter = 0;
    x_res = 320;
    bg_color = 0;
}

uint16_t c_vdp::read_word(uint32_t address)
{
    uint16_t ret;
    switch (address) {
        case 0x00C00000:
        case 0x00C00002: {
            if (address_type != ADDRESS_TYPE::VRAM_READ) {
                //assert(0);
                return 0xFFFF;
            }
            address_write = 0;
            uint16_t ret = vram[_address] << 8;
            ret |= vram[_address + 1];
            _address += reg[0x0F];
            return 0xff;
            return ret;
        }
        case 0x00C00004:
        case 0x00C00006:
            //unsure when dma status should be cleared
            //world series baseball will hang if this reads back set
            //(see 0x1140 in disassembly)
            status.dma = 0;
            ret = status.value;
            status.vint = 0;
            status.dma = 0;
            //*ipl &= ~0x6;
            asserting_vblank = 0;
            asserting_hblank = 0;
            update_ipl();
            address_write = 0;
            return ret;
        case 0x00C00008:
            return (0 << 8) | ((line >> 1) & 0xFF);
        default:
            return 0;
    }
    return 0;
}

void c_vdp::write_word(uint32_t address, uint16_t value)
{
    int x = 0;
    uint16_t a = _address;
    switch (address) {
        case 0x0C00000:
        case 0x0C00002:
            //data
            //OutputDebugString("DATA\n");
            if (address & 0x1) {
                int x = 1;
            }
            if (pending_fill) {
                pending_fill = 0;
                uint16_t len = reg[0x13] | (reg[0x14] << 8);
                vram[_address] = value & 0xFF;
                do {
                    vram[_address ^ 0x1] = (value >> 8) & 0xFF;
                    //vram[_address] = (value >> 8) & 0xFF;
                    _address += reg[0x0F];
                } while (--len);
                return;
            }
            switch (address_type) {
                case ADDRESS_TYPE::VRAM_WRITE:
                    assert(_address < 64 * 1024);
                    vram[_address] = value >> 8;
                    vram[_address + 1] = value & 0xFF;
                    break;
                case ADDRESS_TYPE::CRAM_WRITE:
                    a &= 0x7F;
                    cram[a] = value >> 8;
                    cram[a + 1] = value & 0xFF;
#ifdef USE_BMI2
                    cram_entry[a >> 1] = rgb[_pext_u32(value, 0xEEE)];
#else
                    {
                        uint32_t color = ((value >> 0) & 0xE) * 16;
                        color |= (((value >> 4) & 0xE) * 16) << 8;
                        color |= (((value >> 8) & 0xE) * 16) << 16;
                        color |= 0xFF000000;
                        cram_entry[a >> 1] = color;
                    }
#endif
                    break;
                case ADDRESS_TYPE::VSRAM_WRITE:
                    a &= 0x7F;
                    if (a < 0x50) {
                        vsram[a] = value >> 8;
                        vsram[a + 1] = value & 0xFF;
                    }
                    break;
                default:
                    x = 1;
                    //assert(0);
                    break;
            }
            _address += reg[0x0F];
            if (reg[0x0f] != 2) {
                int x = 1;
            }
            address_write = 0;
            break;
        case 0x0C00004:
        case 0x0C00006:
            //control
            if (value >> 14 == 0x02) {
                uint8_t reg_number = (value >> 8) & 0x1F;
                uint8_t reg_value = value & 0xFF;
                reg[reg_number] = reg_value;
                switch (reg_number) {
                    case 0x07:
                        bg_color = reg_value & 0x3F;
                        break;
                    case 0x10:
                        plane_width = 32 + (reg_value & 0x3) * 32;
                        plane_height = 32 + ((reg_value >> 4) & 0x3) * 32;
                        //Window distortion bug.bin uses invalid plane_width setting
                        if (plane_width == 96) {
                            plane_width = 32;
                            plane_height = 1;
                        }
                        else if (plane_height == 96) {
                            plane_height = 32;
                        }
                        break;
                    case 0x0C:
                        update_x_res();
                        break;
                    default:
                        break;
                }
            }
            else {
                if (address_write) {
                    //second word
                    //address_reg |= value;
                    address_reg = (address_reg & 0xFFFF0000) | value;
                    address_type = (ADDRESS_TYPE)((((address_reg & 0x30) | (address_reg >> 28)) >> 2) & 0xF);
                    _address = ((address_reg & 0x3) << 14) | ((address_reg >> 16) & 0x3FFF);
                    vram_to_vram_copy = address_reg & 0x40;
                    if (vram_to_vram_copy) {
                        int x = 1;
                    }
                    dma_copy = address_reg & 0x80;
                    if (dma_copy) {
                        if (reg[0x01] & 0x10) {
                            status.dma = 1;
                            if (!(reg[0x17] & 0x80)) {
                                do_68k_dma();
                            }
                            else if ((reg[0x17] & 0xC0) == 0xC0) {
                                OutputDebugString("VRAM to VRAM DMA\n");
                            }
                            else if ((reg[0x17] & 0xC0) == 0x80) {
                                pending_fill = 1;
                            }
                        }
                    }
                    x = 1;
                    address_write = 0;
                }
                else {
                    //address_reg = value << 16;
                    address_reg = (address_reg & 0x0000FFFF) | (value << 16);
                    _address = ((address_reg & 0x3) << 14) | ((address_reg >> 16) & 0x3FFF);
                    address_write = 1;
                }
                //address_write ^= 1;
            }
            break;
        default: {
            int x = 1;
        } break;
    }
}

uint8_t c_vdp::read_byte(uint32_t address)
{
    uint8_t ret;
    switch (address) {
        case 0x00C00005:
        case 0x00C00007:
            ret = status.value & 0xFF;
            status.vint = 0;
            status.dma = 0;
            //*ipl &= ~0x6;
            asserting_vblank = 0;
            asserting_hblank = 0;
            update_ipl();
            return ret;
        case 0x00C00008: {
            int r = line;
            if (r > 234) {
                r -= 6;
            }
            return r & 0xFF;
        }
        case 0x00C00009:
            return 0;
        default:
            return 0;
    }
    return 0;
}

void c_vdp::update_x_res()
{
    int current = x_res;
    x_res = reg[0x0C] & 1 ? 320 : 256;
    if (x_res != current) {
        mode_switch_callback(x_res);
    }
}

void c_vdp::write_byte(uint32_t address, uint8_t value)
{
    if (value != 0) {
        int x = 1;
    }
    uint16_t v = (value << 8) | value;
    switch (address) {
        case 0x0C00000:
        case 0x0C00001:
        case 0x0C00002:
        case 0x0C00003:
        case 0x0C00004:
        case 0x0C00005:
        case 0x0C00006:
        case 0x0C00007:
            //not sure if this is correct
            write_word(address & ~1, v);
            break;
        default: {
            int x = 1;
        } break;
    }
}

uint32_t c_vdp::lookup_color(uint32_t pal, uint32_t index)
{
    int loc = pal * 16 + index;
    return cram_entry[loc];
}

void c_vdp::draw_plane(uint8_t *out, uint32_t nt, uint32_t plane_width, uint32_t plane_height, uint32_t v_scroll,
                       uint32_t h_scroll, uint32_t low_pri_val)
{
    uint32_t vscroll_mode = reg[0x0B] & 0x4;
    uint32_t fine_x = (8 - (h_scroll & 0x7)) & 7;
    uint32_t x = 0;
   
    uint8_t *priorities_offset = &priorities[8 - fine_x];

    int column_count = reg[0x0C] & 1 ? 41 : 33;

    for (int column = 0; column < column_count; column++) {
        if (vscroll_mode && out != win_out) {
            int col = column;
            if (fine_x) {
                col--;
            }

            if (col == -1) {
                //https://gendev.spritesmind.net/forum/viewtopic.php?t=737&postdays=0&postorder=asc&start=30
                if (x_res == 320) {
                    v_scroll = ((vsram[0x4C] & vsram[0x4E]) << 8) | (vsram[0x4D] & vsram[0x4F]);
                }
                else {
                    v_scroll = 0;
                }
            }
            else {
                int v_scroll_loc = (col / 2) * 4;
                if (out == a_out) {
                    v_scroll = std::byteswap(*(uint16_t *)&vsram[v_scroll_loc]) & 0x3FF;
                }
                else {
                    v_scroll = std::byteswap(*(uint16_t *)&vsram[v_scroll_loc + 2]) & 0x3FF;
                }
            }
        }

        uint32_t y = line + v_scroll;
        uint32_t y_address = ((y >> 3) & (plane_height - 1)) * plane_width * 2;
        uint32_t nt_column = (((column * 8) - h_scroll) >> 3) & (plane_width - 1);
        uint16_t nt_address = nt + y_address + (nt_column * 2);
        uint16_t tile = std::byteswap(*((uint16_t *)&vram[nt_address]));
        uint32_t tile_number = tile & 0x7FF;
        uint8_t priority = tile >> 15;
        uint32_t h_flip = tile & 0x800 ? 7 : 0;
        uint32_t v_flip = tile & 0x1000 ? 7 : 0;
        uint8_t pal = ((tile >> 13) & 0x3) << 4;
        #ifdef USE_BMI2
        uint64_t palette_broadcast = pal * 0x0101010101010101;
        #endif

        int priority_shift = low_pri_val - (4 * priority);
        int priority_bit = 1 << priority_shift;

        const uint32_t bytes_per_tile = 32;
        const uint32_t bytes_per_tile_row = bytes_per_tile / 8;
        uint32_t pattern_address =
            tile_number * bytes_per_tile + (((y & 7) ^ v_flip) * bytes_per_tile_row);

        uint32_t pattern = std::byteswap(*((uint32_t *)&vram[pattern_address]));        

        #ifdef USE_BMI2
        uint64_t pattern_expanded = _pdep_u64(pattern, 0x0F0F0F0F0F0F0F0F);
        uint64_t pixels = pattern_expanded | palette_broadcast;
        // find non-zero nibbles in p2
        // set the msb to 1
        uint64_t priorities = pattern_expanded | 0x8080808080808080;
        // subtract 1.  If the value of the nibble is zero, there
        // will be a carry from the MSB, setting it to 0
        priorities -= 0x0101010101010101;
        // shift the MSBs into the correct priority position
        priorities &= 0x8080808080808080;
        priorities >>= 7 - priority_shift;

        if (!h_flip) {
            pixels = std::byteswap(pixels);
            priorities = std::byteswap(priorities);
        }

        *((uint64_t *)&priorities_offset[x]) |= priorities;
        *((uint64_t *)&out[x]) = pixels;
        x += 8;
        #else
        for (int p = 0; p < 8; p++) {
            uint8_t pixel = (pattern >> ((7 - (p ^ h_flip)) * 4)) & 0xF;

            if (pixel) {
                priorities_offset[x] |= priority_bit;
                out[x] = pal | pixel;
            }
            x++;
        }
        #endif
    }
}

uint16_t c_vdp::get_hscroll_loc()
{
    uint32_t hscroll_loc = (reg[0x0D] & 0x3F) << 10;
    uint32_t hscroll_offset = line * 4;
    switch (reg[0x0B] & 0x03) {
        case 0:
            hscroll_offset = 0;
            break;
        case 2:
            hscroll_offset = (line & ~7) * 4;
            break;
        case 3:
            hscroll_offset = line * 4;
            break;
        default: {
            int x = 1;
        } break;
    }
    return hscroll_loc + hscroll_offset;
}

void c_vdp::draw_scanline()
{
    if (line < 224) {
        uint32_t vscroll_mode = reg[0x0B] & 0x4;
        uint32_t a_v_scroll = 0;
        uint32_t b_v_scroll = 0;
        if (vscroll_mode == 0) {
            a_v_scroll = std::byteswap(*(uint16_t *)&vsram[0]) & 0x3FF;
            b_v_scroll = std::byteswap(*(uint16_t *)&vsram[2]) & 0x3FF;
        }
        uint32_t *fb = &frame_buffer[line * 320];
        if (!(reg[0x01] & 0x40)) {
            std::fill_n(fb, 320, 0xFF000000);
        }
        else {
            memset(priorities, 0, sizeof(priorities));
            eval_sprites();

            uint8_t vp = reg[0x12] & 0x9F;
            uint8_t hp = reg[0x11] & 0x9F;
            uint32_t in_window = 0;
            const int DISABLE_WINDOW_PLANE = ~((1 << LAYER_PRIORITY::WINDOW_LOW) | (1 << LAYER_PRIORITY::WINDOW_HIGH));
            const int DISABLE_A_PLANE = ~((1 << LAYER_PRIORITY::A_LOW) | (1 << LAYER_PRIORITY::A_HIGH));
            uint32_t plane_mask = DISABLE_WINDOW_PLANE;
            if (vp) {
                int pos = vp & 0x1F;
                
                //if vp & 0x80 (mode), then the window starts at pos and extends to bottom of screen
                //else the window starts at the top of the screen and extends to pos
                //pos of 0 is invalid in the latter mode
                //
                //examples:
                //line = 0, pos = 20, diff = -20
                //line = 20, pos = 20, diff = 0
                //line = 21, pos = 20, diff = 1
                //for the first case, diff is positive when in range
                //for the second case, diff is negative when in range
                //therefore, we can combine the tests by xor'ing the mode
                //bit with the diff sign bit
                
                uint32_t diff = line - (pos * 8);
                int in_range = (diff >> 31) ^ (vp >> 7);

                if (in_range) {
                    in_window = 0x1;
                    plane_mask = DISABLE_A_PLANE;
                }
            }

            uint16_t hscroll_loc = get_hscroll_loc();

            uint16_t a_h_scroll = std::byteswap(*(uint16_t *)&vram[hscroll_loc + 0]) & 0x3FF;
            uint16_t b_h_scroll = std::byteswap(*(uint16_t *)&vram[hscroll_loc + 2]) & 0x3FF;

            uint32_t a_nt = (reg[0x02] & 0x38) << 10;
            uint32_t b_nt = (reg[0x04] & 0x7) << 13;
            uint32_t win_nt = (reg[0x03] & 0x3E) << 10;

            draw_plane(a_out, a_nt, plane_width, plane_height, a_v_scroll, a_h_scroll, LAYER_PRIORITY::A_LOW);
            draw_plane(b_out, b_nt, plane_width, plane_height, b_v_scroll, b_h_scroll, LAYER_PRIORITY::B_LOW);
            draw_plane(win_out, win_nt, reg[0x0C] & 1 ? 64 : 32, 32, 0, 0, LAYER_PRIORITY::WINDOW_LOW);

            uint32_t a_fine_x = (8 - (a_h_scroll & 0x7)) & 7;
            uint32_t b_fine_x = (8 - (b_h_scroll & 0x7)) & 7;
            plane_ptrs[2] = a_out + a_fine_x;
            plane_ptrs[3] = b_out + b_fine_x;

            for (int i = 0; i < x_res; i += 16) {
                if (hp && !(in_window & 1)) {
                    int pos = hp & 0x1F;
                    uint32_t diff = i - (pos * 16);
                    int in_range = (diff >> 31) ^ (hp >> 7);

                    if (in_range) {
                        in_window |= 0x2;
                        plane_mask = DISABLE_A_PLANE;
                    }
                    else if (in_window & 0x2) {
                        if (a_h_scroll & 0xF) {
                            //emulate window bug
                            //pretty kludgey...
                            for (int j = i; j < i + (a_h_scroll & 0xF); j++) {
                                int src = (j + 16) % x_res;
                                priorities[j + 8] &= DISABLE_A_PLANE;
                                priorities[j + 8] |= priorities[src + 8] & ~DISABLE_A_PLANE;
                                plane_ptrs[2][j] = plane_ptrs[2][src];
                            }
                        }
                        in_window &= ~0x2;
                        if (!in_window) {
                            plane_mask = DISABLE_WINDOW_PLANE;
                        }
                    }
                }
                for (int j = 0; j < 16; j++) {
                    uint32_t p = priorities[8 + i + j] & plane_mask;
                    uint8_t out;

                    if (p) {
                        uint32_t c = _tzcnt_u32(p) & 0x3;
                        out = plane_ptrs[c][j + i];
                    }
                    else {
                        out = bg_color;
                    }
                    *fb++ = cram_entry[out];
                }
            }
        }
    }

    if (--hint_counter == 0) {
        if (reg[0x00] & 0x10) {
            asserting_hblank = 1;
            status.hblank = 1;
            update_ipl();
            hint_counter = reg[0x0A];
        }
    }

    if (line == 224) {
        status.vblank = 1;
        if (reg[0x01] & 0x20) {
            status.vint = 1;
            //*ipl |= 0x6;
            asserting_vblank = 1;
            status.vblank = 1;
            update_ipl();
        }
    }
    else if (line == 255) {
        status.vblank = 0;
        //does interrupt get cleared automatically?
        status.vint = 0;
    }

    if (line == 261) {
        line = 0;
        hint_counter = reg[0x0A];
    }
    else {
        line++;
    }
}

void c_vdp::eval_sprites()
{
    uint16_t sprite_table_base = (reg[0x05] & (x_res == 320 ? 0x7E : 0x7F)) << 9;

    memset(sprite_out, -1, sizeof(sprite_out));


    const uint32_t sprites_per_frame = (x_res == 320 ? 80 : 64);
    const uint32_t sprites_per_scanline = (x_res == 320 ? 20 : 16);
    uint32_t sprite_number = 0;
    uint32_t sprite_count = 0;
    uint32_t mask_low_priority = 0;
    uint32_t pixels_drawn = 0;
    uint32_t first_sprite = 1;
    for (int i = 0; i < sprites_per_frame; i++) {
        uint16_t attribute1 = std::byteswap(*(uint16_t *)&vram[sprite_table_base + sprite_number * 8 + 0]);
        uint16_t attribute2 = std::byteswap(*(uint16_t *)&vram[sprite_table_base + sprite_number * 8 + 2]);
        uint16_t attribute3 = std::byteswap(*(uint16_t *)&vram[sprite_table_base + sprite_number * 8 + 4]);
        uint16_t attribute4 = std::byteswap(*(uint16_t *)&vram[sprite_table_base + sprite_number * 8 + 6]);
        //todo: vpos is lower 10 bits of first word.  truxtron requires mask of 1ff to work properly.
        //per https://wiki.megadrive.org/index.php?title=VDP_Sprites, non-interlaced range is 0-511,
        //interlaced range is 0-1023.  Mask probably needs to change depending on mode, but need to verify.
        uint32_t vpos = attribute1 & 0x1FF;
        uint32_t hpos = attribute4 & 0x1FF;
        uint32_t next = attribute2 & 0x7F;
        uint32_t vsize = ((attribute2 >> 8) & 0x3);
        uint32_t hsize = ((attribute2 >> 10) & 0x3);
        uint32_t base_tile = attribute3 & 0x7FF;
        uint32_t h_flip = attribute3 & 0x800 ? 7 : 0;
        uint32_t v_flip = attribute3 & 0x1000 ? 7 : 0;
        uint32_t h_xor = (h_flip && hsize) ? hsize + 1 : 0;
        uint32_t palette = ((attribute3 >> 13) & 3) << 4;
        bool priority = attribute3 & 0x8000;

        int32_t y = (int32_t)vpos - 128;
        int32_t x = (int32_t)hpos - 128;

        if (mask_low_priority) {
            continue;
        }

        uint32_t sprite_here = 0;

        for (int v = 0; v < vsize + 1; v++) {
            int32_t y_base = y + v * 8;
            if (line >= y_base && line < y_base + 8) {
                if (sprite_number == 9) {
                    int x = 1;
                }
                if (hpos == 0) {
                    if (i != 0) {
                        mask_low_priority = 1;
                    }
                }

                /*if (!first_sprite && hpos == 0) {
                    mask_low_priority = 1;
                }*/
                sprite_here = 1;
                first_sprite = 0;
                int32_t y_offset = line - y_base;
                uint32_t vv = (v_flip && vsize) ? vsize - v : v;
                for (int h = 0; h < hsize + 1; h++) {
                    //load tile at this position
                    uint32_t hh = (h_flip && hsize) ? hsize - h : h;
                    uint32_t tile = base_tile + vv + (hh * (vsize + 1));
                    uint32_t pattern_address = tile * 32 + ((y_offset ^ v_flip) * 4);
                    if (pattern_address == 0xaa80) {
                        int x = 1;
                    }
                    uint32_t pattern = std::byteswap(*((uint32_t *)&vram[pattern_address]));
                    int x_start = x + h * 8;
                    for (int j = x_start, p = 0; j < x_start + 8; j++, p++) {
                        if (++pixels_drawn > x_res) {
                            return;
                        }
                        if (j >= 0 && j < x_res) {
                            if (sprite_out[j] != 0xFF) {
                                //there is already a sprite here
                                if ((sprite_out[j] & 0xF) != 0) {
                                    //and it's not transparent
                                    continue;
                                }
                            }
                            uint8_t color = (pattern >> ((7 - (p ^ h_flip)) * 4)) & 0xF;
                            if (color) {
                                if (priority) {
                                    priorities[j + 8] |= (1 << LAYER_PRIORITY::SPRITE_HIGH);
                                }
                                else {
                                    priorities[j + 8] |= (1 << LAYER_PRIORITY::SPRITE_LOW);
                                }
                                sprite_out[j] = palette | color;
                            }
                        }
                    }
                }
                break;
            }
        }

        sprite_count += sprite_here;

        if (next == 0 || sprite_count == sprites_per_scanline) {
            break;
        }
        sprite_number = next;
    }
}

void c_vdp::clear_hblank()
{
    status.hblank = 0;
    asserting_hblank = 0;
    update_ipl();
}

void c_vdp::ack_irq()
{
    if (*ipl == 4) {
        asserting_hblank = 0;
    }
    else if (*ipl == 6) {
        asserting_vblank = 0;
    }
    update_ipl();
}

void c_vdp::update_ipl()
{
    if (asserting_hblank) {
        *ipl = 4;
    }
    else if (asserting_vblank) {

        *ipl = 6;
    }
    else {
        *ipl = 0;
    }
}

void c_vdp::do_68k_dma()
{
    uint16_t len = reg[0x13] | (reg[0x14] << 8);
    if (len == 0) {
        len = 0xFFFF;
    }
    *stalled = len * 3;
    uint32_t src = reg[0x15] | (reg[0x16] << 8) | ((reg[0x17] & 0x7F) << 16);
    if (src & 0x1) {
        int x = 1;
    }
    src <<= 1;
    src &= ~1;
    if (address_type == ADDRESS_TYPE::CRAM_WRITE) {
        int x = 1;
    }
    do {
        uint16_t data = read_word_68k(src);
        src += 2;
        if (src == 0x01000000) {
            src = 0xFF0000;
        }
        write_word(0x0C00000, data);

        len--;
    } while (len != 0);
    int x = 1;
}

} //namespace genesis