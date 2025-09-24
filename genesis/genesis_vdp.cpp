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

    plane_ptrs[0] = sprite_out;
    plane_ptrs[1] = win_out;
    plane_ptrs[2] = a_out;
    plane_ptrs[3] = b_out;
}

constexpr auto c_vdp::make_rgb_entries()
{
    std::array<std::array<uint32_t, 512>, 3> entries{};
    //colors from
    //https://forums.nesdev.org/viewtopic.php?p=233530&sid=e77eb32ab8b8a8da89b7f7cf77e7cfa7#p233530
    uint8_t colors[3][8] = {
        {0,  29,  52,  70,  87, 101, 116, 130}, //shadow
        {0,  52,  87, 116, 144, 172, 206, 255}, //normal
      {130, 144, 158, 172, 187, 206, 228, 255}  //highlight
    };

    for (int color_mode = 0; color_mode < 3; color_mode++) {
        for (int i = 0; i < 512; i++) {
            uint32_t color = 0xFF000000;
            color |= colors[color_mode][(i >> 0) & 7] << 0;
            color |= colors[color_mode][(i >> 3) & 7] << 8;
            color |= colors[color_mode][(i >> 6) & 7] << 16;
            entries[color_mode][i] = color;
        }
    }
    return entries;
}

constexpr std::array<std::array<uint32_t, 512>, 3> c_vdp::rgb_entries = make_rgb_entries();

c_vdp::~c_vdp()
{
}

void c_vdp::reset()
{
    status.value = 0;
    line = 0;
    std::memset(reg, 0, sizeof(reg));
    address_reg = 0;
    address_write = 0;
    memset(vram, -1, sizeof(vram));
    memset(cram, 0, sizeof(cram));
    memset(vsram, 0, sizeof(vsram));
    std::fill_n(frame_buffer, 320 * 224, 0xFF000000);
    std::fill_n((uint32_t*)cram_entries, 3 * 128, 0xFF000000);
    line = 0;
    freeze_cpu = 0;
    pending_fill = 0;
    asserting_hblank = 0;
    asserting_vblank = 0;
    update_ipl();
    hint_counter = 0;
    x_res = next_x_res = 320;
    bg_color = 0;
    event = 0;
    vscroll_a = 0;
    vscroll_b = 0;
    event_index = 0;
    current_cycle = 0;
    hpos = 0;
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
            return ret;
        }
        case 0x00C00004:
        case 0x00C00006:
            status.dma = 0;
            ret = status.value;
            status.vint = 0;
            address_write = 0;
            return ret;
        case 0x00C00008:
            //hpos is inaccurate, but good enough to get comix zone to work
            //need to revisit
            return (line > 0xEA ? line - 6 : line) << 8 | (hpos >> 1);
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
                    for (int i = 0; i < 3; i++) {
                        cram_entries[i][a >> 1] = rgb_entries[i][_pext_u32(value, 0xEEE)];
                    }
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
            address_write = 0;
            break;
        case 0x0C00004:
        case 0x0C00006:
            //control
            if (value >> 14 == 0x02) {
                uint8_t reg_number = (value >> 8) & 0x1F;
                uint8_t reg_value = value & 0xFF;
                address_type = ADDRESS_TYPE::INVALID;
                reg[reg_number] = reg_value;
                switch (reg_number) {
                    case 0x00:
                    case 0x01:
                        //update_ipl();
                        break;
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
                        //update_x_res();
                        next_x_res = reg[0x0C] & 1 ? 320 : 256;
                        break;
                    default:
                        break;
                }
            }
            else {
                if (address_write) {
                    //second word
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
                    address_reg = (address_reg & 0x0000FFFF) | (value << 16);
                    _address = ((address_reg & 0x3) << 14) | ((address_reg >> 16) & 0x3FFF);
                    address_write = 1;
                }
            }
            break;
        default:
            break;
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
}

void c_vdp::update_x_res()
{
    x_res = next_x_res;
    mode_switch_callback(x_res);
}

void c_vdp::write_byte(uint32_t address, uint8_t value)
{
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
        default:
            break;
    }
}

void c_vdp::draw_plane(uint8_t *out, uint32_t nt, uint32_t plane_width, uint32_t plane_height, uint32_t v_scroll,
                       uint32_t h_scroll, uint32_t low_pri_val)
{
    uint32_t vscroll_mode = reg[0x0B] & 0x4;
    uint32_t fine_x = (8 - (h_scroll & 0x7)) & 7;
    uint32_t x = 0;
   
    uint8_t *visibility_offset = &layer_visibility[8 - fine_x];
    uint8_t *priority_offset = &layer_priorities[8 - fine_x];

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

        int layer_shift = low_pri_val - (4 * priority);
        int priority_bit = 1 << layer_shift;

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
        uint64_t visibility = pattern_expanded | 0x8080808080808080;
        // subtract 1.  If the value of the nibble is zero, there
        // will be a carry from the MSB, setting it to 0
        visibility -= 0x0101010101010101;
        // shift the MSBs into the correct priority position
        visibility &= 0x8080808080808080;
        visibility >>= 7 - layer_shift;

        uint64_t priorities = priority ? 0x8080808080808080 : 0;
        priorities >>= 7 - layer_shift;
        *((uint64_t *)&priority_offset[x]) |= priorities;

        if (!h_flip) {
            pixels = std::byteswap(pixels);
            visibility = std::byteswap(visibility);
        }

        *((uint64_t *)&visibility_offset[x]) |= visibility;
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
        default:
            assert(0);
            break;
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
            a_v_scroll = vscroll_a;
            b_v_scroll = vscroll_b;
        }
        uint32_t *fb = &frame_buffer[line * 320];
        if (!(reg[0x01] & 0x40)) {
            std::fill_n(fb, 320, cram_entries[1][bg_color]);
        }
        else {
            memset(layer_visibility, 0, sizeof(layer_visibility));
            memset(layer_priorities, 0, sizeof(layer_priorities));
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
                                layer_visibility[j + 8] &= DISABLE_A_PLANE;
                                layer_visibility[j + 8] |= layer_visibility[src + 8] & ~DISABLE_A_PLANE;
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
                    uint8_t visible_layers = layer_visibility[8 + i + j] & plane_mask;
                    uint8_t out;
                    int color_mode = 1;
                    if (visible_layers) {
                        uint32_t visible_layer = _tzcnt_u32(visible_layers);
                        out = plane_ptrs[visible_layer & 0x3][j + i];
                        
                        //shadow/highlight mode
                        //https://gendev.spritesmind.net/forum/viewtopic.php?p=32174&sid=469a79afbb947556542ed3e4ff6c3f78#p32174
                        if (reg[0x0C] & 0x8) {
                            uint8_t high_priority_layers = layer_priorities[8 + i + j] & plane_mask & 0xE;
                            color_mode = high_priority_layers ? 1 : 0;

                            auto mask_sprite = [&]() {
                                //v &= ~0x11;
                                visible_layers &= ~((1 << SPRITE_HIGH) | (1 << SPRITE_LOW));
                                out = visible_layers ? plane_ptrs[_tzcnt_u32(visible_layers) & 0x3][j + i] : bg_color;
                            };

                            if (visible_layer == SPRITE_HIGH || visible_layer == SPRITE_LOW) {
                                switch (out) {
                                    case 0x0E:
                                    case 0x1E:
                                    case 0x2E:
                                        color_mode = 1;
                                        break;
                                    case 0x3E:
                                        color_mode += 1;
                                        mask_sprite();
                                        break;
                                    case 0x3F:
                                        color_mode = 0;
                                        mask_sprite();
                                        break;
                                    default:
                                        if (visible_layer == SPRITE_HIGH) {
                                            color_mode = 1;
                                        }
                                }
                            }
                        }
                    }
                    else {
                        out = bg_color;
                    }
                    *fb++ = cram_entries[color_mode][out];
                }
            }
        }
    }
}

void c_vdp::end_line()
{
    clear_hblank();
    if (line == 223) {
        status.vblank = 1;
        if (reg[0x01] & 0x20) {
            status.vint = 1;
            asserting_vblank = 1;
            status.vblank = 1;
            update_ipl();
        }
    }

    if (line == 261) {
        line = 0;
        hint_counter = reg[0x0A];
    }
    else {
        line++;
        if (line == 261) {
            status.vblank = 0;
            status.vint = 0;
        }
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
                if (hpos == 0) {
                    if (i != 0) {
                        mask_low_priority = 1;
                    }
                }

                sprite_here = 1;
                first_sprite = 0;
                int32_t y_offset = line - y_base;
                uint32_t vv = (v_flip && vsize) ? vsize - v : v;
                for (int h = 0; h < hsize + 1; h++) {
                    //load tile at this position
                    uint32_t hh = (h_flip && hsize) ? hsize - h : h;
                    uint32_t tile = base_tile + vv + (hh * (vsize + 1));
                    uint32_t pattern_address = tile * 32 + ((y_offset ^ v_flip) * 4);
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
                                    layer_visibility[j + 8] |= (1 << LAYER_PRIORITY::SPRITE_HIGH);
                                }
                                else {
                                    layer_visibility[j + 8] |= (1 << LAYER_PRIORITY::SPRITE_LOW);
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
    if (*ipl == 6) {
        asserting_vblank = 0;
    }
    else if (*ipl == 4) {
        asserting_hblank = 0;
    }
    update_ipl();
}

void c_vdp::update_ipl()
{
    if (asserting_vblank) {
        *ipl = 6;
    }
    else if (asserting_hblank) {

        *ipl = 4;
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

    //Analog screen sections in relation to HCounter (H32 mode):
//-----------------------------------------------------------------
//| Screen section | HCounter  |Pixel| Pixel |Serial|Serial |MCLK |
//| (PAL/NTSC H32) |  value    |clock| clock |clock |clock  |ticks|
//|                |           |ticks|divider|ticks |divider|     |
//|----------------|-----------|-----|-------|------|-------|-----|
//|Left border     |0x00B-0x017|  13 |SCLK/2 |   26 |MCLK/5 | 130 |
//|----------------|-----------|-----|-------|------|-------|-----|
//|Active display  |0x018-0x117| 256 |SCLK/2 |  512 |MCLK/5 |2560 |
//|----------------|-----------|-----|-------|------|-------|-----|
//|Right border    |0x118-0x125|  14 |SCLK/2 |   28 |MCLK/5 | 140 |
//|----------------|-----------|-----|-------|------|-------|-----|
//|Back porch      |0x126-0x127|   9 |SCLK/2 |   18 |MCLK/5 |  90 |
//|(Right Blanking)|0x1D2-0x1D8|     |       |      |       |     |
//|----------------|-----------|-----|-------|------|-------|-----|
//|Horizontal sync |0x1D9-0x1F2|  26 |SCLK/2 |   52 |MCLK/5 | 260 |
//|----------------|-----------|-----|-------|------|-------|-----|
//|Front porch     |0x1F3-0x00A|  24 |SCLK/2 |   48 |MCLK/5 | 240 |
//|(Left Blanking) |           |     |       |      |       |     |
//|----------------|-----------|-----|-------|------|-------|-----|
//|TOTALS          |           | 342 |       |  684 |       |3420 |
//-----------------------------------------------------------------

//Analog screen sections in relation to HCounter (H40 mode):
//--------------------------------------------------------------------
//| Screen section |   HCounter    |Pixel| Pixel |EDCLK| EDCLK |MCLK |
//| (PAL/NTSC H40) |    value      |clock| clock |ticks|divider|ticks|
//|                |               |ticks|divider|     |       |     |
//|----------------|---------------|-----|-------|-----|-------|-----|
//|Left border     |0x00D-0x019    |  13 |EDCLK/2|  26 |MCLK/4 | 104 |
//|----------------|---------------|-----|-------|-----|-------|-----|
//|Active display  |0x01A-0x159    | 320 |EDCLK/2| 640 |MCLK/4 |2560 |
//|----------------|---------------|-----|-------|-----|-------|-----|
//|Right border    |0x15A-0x167    |  14 |EDCLK/2|  28 |MCLK/4 | 112 |
//|----------------|---------------|-----|-------|-----|-------|-----|
//|Back porch      |0x168-0x16C    |   9 |EDCLK/2|  18 |MCLK/4 |  72 |
//|(Right Blanking)|0x1C9-0x1CC    |     |       |     |       |     |
//|----------------|---------------|-----|-------|-----|-------|-----|
//|Horizontal sync |0x1CD.0-0x1D4.5| 7.5 |EDCLK/2|  15 |MCLK/5 |  75 |
//|                |0x1D4.5-0x1D5.5|   1 |EDCLK/2|   2 |MCLK/4 |   8 |
//|                |0x1D5.5-0x1DC.0| 7.5 |EDCLK/2|  15 |MCLK/5 |  75 |
//|                |0x1DD.0        |   1 |EDCLK/2|   2 |MCLK/4 |   8 |
//|                |0x1DE.0-0x1E5.5| 7.5 |EDCLK/2|  15 |MCLK/5 |  75 |
//|                |0x1E5.5-0x1E6.5|   1 |EDCLK/2|   2 |MCLK/4 |   8 |
//|                |0x1E6.5-0x1EC.0| 6.5 |EDCLK/2|  13 |MCLK/5 |  65 |
//|                |===============|=====|=======|=====|=======|=====|
//|        Subtotal|0x1CD-0x1EC    | (32)|       | (64)|       |(314)|
//|----------------|---------------|-----|-------|-----|-------|-----|
//|Front porch     |0x1ED          |   1 |EDCLK/2|   2 |MCLK/5 |  10 |
//|(Left Blanking) |0x1EE-0x00C    |  31 |EDCLK/2|  62 |MCLK/4 | 248 |
//|                |===============|=====|=======|=====|=======|=====|
//|        Subtotal|0x1ED-0x00C    | (32)|       | (64)|       |(258)|
//|----------------|---------------|-----|-------|-----|-------|-----|
//|TOTALS          |               | 420 |       | 840 |       |3420 |
//--------------------------------------------------------------------

//Digital render events in relation to HCounter:
//----------------------------------------------------
//|        Video |PAL/NTSC         |PAL/NTSC         |
//|         Mode |H32     (RSx=00) |H40     (RSx=11) |
//|              |V28/V30 (M2=*)   |V28/V30 (M2=*)   |
//| Event        |Int any (LSMx=**)|Int any (LSMx=**)|
//|--------------------------------------------------|
//|HCounter      |[1]0x000-0x127   |[1]0x000-0x16C   |
//|progression   |[2]0x1D2-0x1FF   |[2]0x1C9-0x1FF   |
//|9-bit internal|                 |                 |
//|--------------------------------------------------|
//|VCounter      |HCounter changes |HCounter changes |
//|increment     |from 0x109 to    |from 0x149 to    |
//|              |0x10A in [1].    |0x14A in [1].    |
//|--------------------------------------------------| //Logic analyzer tests conducted on 2012-11-03 confirm 18 SC
//|HBlank set    |HCounter changes |HCounter changes | //cycles between HBlank set in status register and HSYNC
//|              |from 0x125 to    |from 0x165 to    | //asserted in H32 mode, and 21 SC cycles in H40 mode.
//|              |0x126 in [1].    |0x166 in [1].    | //Note this actually means in H40 mode, HBlank is set at
//0x166.5.
//|--------------------------------------------------| //Logic analyzer tests conducted on 2012-11-03 confirm 46 SC
//|HBlank cleared|HCounter changes |HCounter changes | //cycles between HSYNC cleared and HBlank cleared in status
//|              |from 0x009 to    |from 0x00A to    | //register in H32 mode, and 61 SC cycles in H40 mode.
//|              |0x00A in [1].    |0x00B in [1].    | //Note this actually means in H40 mode, HBlank is cleared at
//0x00B.5.
//|--------------------------------------------------|
//|F flag set    |HCounter changes |HCounter changes | //Logic analyzer tests conducted on 2012-11-03 confirm 28 SC
//|              |from 0x000 to    |from 0x000 to    | //cycles between HSYNC cleared and odd flag toggled in status
//|              |0x001 in [1]     |0x001 in [1]     | //register in H32 mode, and 40 SC cycles in H40 mode.
//|--------------------------------------------------|
//|ODD flag      |HCounter changes |HCounter changes | //Logic analyzer tests conducted on 2012-11-03 confirm 30 SC
//|toggled       |from 0x001 to    |from 0x001 to    | //cycles between HSYNC cleared and odd flag toggled in status
//|              |0x002 in [1]     |0x002 in [1]     | //register in H32 mode, and 42 SC cycles in H40 mode.
//|--------------------------------------------------|
//|HINT flagged  |HCounter changes |HCounter changes | //Logic analyzer tests conducted on 2012-11-02 confirm 74 SC
//|via IPL lines |from 0x109 to    |from 0x149 to    | //cycles between HINT flagged in IPL lines and HSYNC
//|              |0x10A in [1].    |0x14A in [1].    | //asserted in H32 mode, and 78 SC cycles in H40 mode.
//|--------------------------------------------------|
//|VINT flagged  |HCounter changes |HCounter changes | //Logic analyzer tests conducted on 2012-11-02 confirm 28 SC
//|via IPL lines |from 0x000 to    |from 0x000 to    | //cycles between HSYNC cleared and VINT flagged in IPL lines
//|              |0x001 in [1].    |0x001 in [1].    | //in H32 mode, and 40 SC cycles in H40 mode.
//|--------------------------------------------------|
//|HSYNC asserted|HCounter changes |HCounter changes |
//|              |from 0x1D8 to    |from 0x1CC to    |
//|              |0x1D9 in [2].    |0x1CD in [2].    |
//|--------------------------------------------------|
//|HSYNC negated |HCounter changes |HCounter changes |
//|              |from 0x1F2 to    |from 0x1EC to    |
//|              |0x1F3 in [2].    |0x1ED in [2].    |
//----------------------------------------------------

//Analog screen sections in relation to VCounter:
//-------------------------------------------------------------------------------------------
//|           Video |NTSC             |NTSC             |PAL              |PAL              |
//|            Mode |H32/H40(RSx00/11)|H32/H40(RSx00/11)|H32/H40(RSx00/11)|H32/H40(RSx00/11)|
//|                 |V28     (M2=0)   |V30     (M2=1)   |V28     (M2=0)   |V30     (M2=1)   |
//|                 |Int none(LSMx=*0)|Int none(LSMx=*0)|Int none(LSMx=*0)|Int none(LSMx=*0)|
//|                 |------------------------------------------------------------------------
//|                 | VCounter  |Line | VCounter  |Line | VCounter  |Line | VCounter  |Line |
//| Screen section  |  value    |count|  value    |count|  value    |count|  value    |count|
//|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
//|Active display   |0x000-0x0DF| 224 |0x000-0x1FF| 240*|0x000-0x0DF| 224 |0x000-0x0EF| 240 |
//|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
//|Bottom border    |0x0E0-0x0E7|   8 |           |   0 |0x0E0-0x0FF|  32 |0x0F0-0x107|  24 |
//|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
//|Bottom blanking  |0x0E8-0x0EA|   3 |           |   0 |0x100-0x102|   3 |0x108-0x10A|   3 |
//|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
//|Vertical sync    |0x1E5-0x1E7|   3 |           |   0 |0x1CA-0x1CC|   3 |0x1D2-0x1D4|   3 |
//|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
//|Top blanking     |0x1E8-0x1F4|  13 |           |   0 |0x1CD-0x1D9|  13 |0x1D5-0x1E1|  13 |
//|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
//|Top border       |0x1F5-0x1FF|  11 |           |   0 |0x1DA-0x1FF|  38 |0x1E2-0x1FF|  30 |
//|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
//|TOTALS           |           | 262 |           | 240*|           | 313 |           | 313 |
//-------------------------------------------------------------------------------------------
//*When V30 mode and NTSC mode are both active, no border, blanking, or retrace
//occurs. A 30-row display is setup and rendered, however, immediately following the
//end of the 30th row, the 1st row starts again. In addition, the VCounter is never
//reset, which usually happens at the beginning of vertical blanking. Instead, the
//VCounter continuously counts from 0x000-0x1FF, then wraps around back to 0x000 and
//begins again. Since there are only 240 lines output as part of the display, this
//means the actual line being rendered is desynchronized from the VCounter. Digital
//events such as vblank flags being set/cleared, VInt being triggered, the odd flag
//being toggled, and so forth, still occur at the correct VCounter positions they
//would occur in (IE, the same as PAL mode V30), however, since the VCounter has 512
//lines per cycle, this means VInt is triggered at a slower rate than normal.
//##TODO## Confirm on the hardware that the rendering row is desynchronized from the
//VCounter. This would seem unlikely, since a separate render line counter would have
//to be maintained apart from VCounter for this to occur.

//Digital render events in relation to VCounter under NTSC mode:
//#ODD - Runs only when the ODD flag is set
//----------------------------------------------------------------------------------------
//|        Video |NTSC             |NTSC             |NTSC             |NTSC             |
//|         Mode |H32/H40(RSx00/11)|H32/H40(RSx00/11)|H32/H40(RSx00/11)|H32/H40(RSx00/11)|
//|              |V28     (M2=0)   |V28     (M2=0)   |V30     (M2=1)   |V30     (M2=1)   |
//| Event        |Int none(LSMx=*0)|Int both(LSMx=*1)|Int none(LSMx=*0)|Int both(LSMx=*1)|
//|--------------------------------------------------------------------------------------|
//|VCounter      |[1]0x000-0x0EA   |[1]0x000-0x0EA   |[1]0x000-0x1FF   |[1]0x000-0x1FF   |
//|progression   |[2]0x1E5-0x1FF   |[2]0x1E4(#ODD)   |                 |                 |
//|9-bit internal|                 |[3]0x1E5-0x1FF   |                 |                 |
//|--------------------------------------------------------------------------------------|
//|VBlank set    |VCounter changes |                 |VCounter changes |                 |
//|              |from 0x0DF to    |     <Same>      |from 0x0EF to    |     <Same>      |
//|              |0x0E0 in [1].    |                 |0x0F0 in [1].    |                 |
//|--------------------------------------------------------------------------------------|
//|VBlank cleared|VCounter changes |                 |VCounter changes |                 |
//|              |from 0x1FE to    |     <Same>      |from 0x1FE to    |     <Same>      |
//|              |0x1FF in [2].    |                 |0x1FF in [1].    |                 |
//|--------------------------------------------------------------------------------------|
//|F flag set    |At indicated     |                 |At indicated     |                 |
//|              |HCounter position|                 |HCounter position|                 |
//|              |while VCounter is|     <Same>      |while VCounter is|     <Same>      |
//|              |set to 0x0E0 in  |                 |set to 0x0F0 in  |                 |
//|              |[1].             |                 |[1].             |                 |
//|--------------------------------------------------------------------------------------|
//|VSYNC asserted|VCounter changes |                 |      Never      |                 |
//|              |from 0x0E7 to    |     <Same>      |                 |     <Same>      |
//|              |0x0E8 in [1].    |                 |                 |                 |
//|--------------------------------------------------------------------------------------|
//|VSYNC cleared |VCounter changes |                 |      Never      |                 |
//|              |from 0x1F4 to    |     <Same>      |                 |     <Same>      |
//|              |0x1F5 in [2].    |                 |                 |                 |
//|--------------------------------------------------------------------------------------|
//|ODD flag      |At indicated     |                 |At indicated     |                 |
//|toggled       |HCounter position|                 |HCounter position|                 |
//|              |while VCounter is|     <Same>      |while VCounter is|     <Same>      |
//|              |set to 0x0E0 in  |                 |set to 0x0F0 in  |                 |
//|              |[1].             |                 |[1].             |                 |
//----------------------------------------------------------------------------------------

uint32_t c_vdp::do_event()
{
    enum EVENT
    {
        START_LINE,
        HBLANK_CLEAR,
        HINT,
        HBLANK_SET,
        LATCH
    };

    struct s_events
    {
        uint32_t next_mcycle;
        uint8_t next_event;
    };

    static const s_events events[][5] = {
        {
            {0xA * 8, HBLANK_CLEAR},
            {0x149 * 8, HINT},
            {0x165 * 8, HBLANK_SET},
            {0x168 * 8, LATCH},
            {3420, START_LINE},
        },
        {
            {0x9 * 10, HBLANK_CLEAR},
            {0x109 * 10, HINT},
            {0x121 * 10, LATCH},
            {0x125 * 10, HBLANK_SET},
            {3420, START_LINE},
        }
    };

    switch (event) {
        case START_LINE:
            event_index = 0;
            current_cycle = 0;
            if (line == 0) {
                if (x_res != next_x_res) {
                    update_x_res();
                }
            }
            if (line == 224) {
                if (reg[0x01] & 0x20) {
                    status.vint = 1;
                    asserting_vblank = 1;
                    update_ipl();
                }
            }
            break;
        case HBLANK_CLEAR:
            status.hblank = 0;
            break;
        case HINT:
            if (line < 224) {
                draw_scanline();
                if (--hint_counter == 0xFF) {
                    if (reg[0x00] & 0x10) {
                        asserting_hblank = 1;
                        update_ipl();
                        hint_counter = reg[0x0A];
                    }
                }
            }
            line++;
            if (line == 224) {
                status.vblank = 1;
            }
            if (line > 223) {
                hint_counter = reg[0x0A];
            }
            if (line == 262) {
                line = 0;
                status.vblank = 0;
                status.vint = 0;
            }
            break;
        case HBLANK_SET:
            if (line <= 224) {
                status.hblank = 1;
            }
            break;
        case LATCH:
            vscroll_a = std::byteswap(*(uint16_t *)&vsram[0]) & 0x3FF;
            vscroll_b = std::byteswap(*(uint16_t *)&vsram[2]) & 0x3FF;
            break;

    }
    hpos = current_cycle;
    int res = x_res == 256;
    uint32_t diff = events[res][event_index].next_mcycle - current_cycle;
    event = events[res][event_index].next_event;
    current_cycle = events[res][event_index].next_mcycle;
    event_index++;
    return diff;
}

} //namespace genesis