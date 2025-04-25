module;
#include <cassert>
#include <Windows.h>
module genesis:vdp;

namespace genesis
{

c_vdp::c_vdp(uint8_t *ipl, read_word_t read_word_68k, uint32_t *stalled)
{
    this->stalled = stalled;
    this->ipl = ipl;
    this->read_word_68k = read_word_68k;
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
    memset(vram, 0, sizeof(vram));
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
}

uint16_t c_vdp::read_word(uint32_t address)
{
    uint16_t ret;
    switch (address) {
        case 0x00C00000:
        case 0x00C00002:
            address_write = 0;
            break;
        case 0x00C00004:
        case 0x00C00006:
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
            if (value >> 14 == 0x02) 
            {
                uint8_t reg_number = (value >> 8) & 0x1F;
                uint8_t reg_value = value & 0xFF;
                reg[reg_number] = reg_value;
                int x = 1;
            }
            else {
                if (address_write) {
                    //second word
                    //address_reg |= value;
                    address_reg = (address_reg & 0xFFFF0000) | value;
                    address_type = (ADDRESS_TYPE)((((address_reg & 0xF0) | (address_reg >> 28)) >> 2) & 0xF);
                    _address = ((address_reg & 0x3) << 14) | ((address_reg >> 16) & 0x3FFF);
                    vram_to_vram_copy = address_reg & 0x40;
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
        }
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
            //*ipl &= ~0x6;
            asserting_vblank = 0;
            asserting_hblank = 0;
            update_ipl();
            return ret;
        case 0x00C00008:
            return 0; //what should this return?
        default:
            return 0;
    }
    return 0;
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
        }
            break;
    }
}

uint32_t c_vdp::lookup_color(uint32_t pal, uint32_t index)
{
    int loc = pal * 32 + index * 2;
    uint16_t entry = (cram[loc] << 8) | cram[loc + 1];
    uint32_t color = ((entry >> 0) & 0xE) * 16;
    color |= (((entry >> 4) & 0xE) * 16) << 8;
    color |= (((entry >> 8) & 0xE) * 16) << 16;
    color |= 0xFF000000;
    return color;
}

void c_vdp::draw_scanline()
{
    int y = line;
    //int plane_height = (reg[0x10] >> 4) & 0x3;
    //int plane_width = reg[0x10] & 0x3;

    int plane_width = 32;
    int plane_height = 32;

    switch ((reg[0x10] >> 4) & 0x3) {
        case 1:
            plane_height = 64;
            break;
        case 3:
            plane_height = 128;
            break;
        default:
            break;
    }
    switch (reg[0x10] & 0x3) {
        case 1:
            plane_width = 64;
            break;
        case 3:
            plane_width = 128;
            break;
        default:
            break;
    }

    uint32_t vscroll_mode = reg[0x0B] & 0x4;
    uint32_t a_v_scroll = 0;
    uint32_t b_v_scroll = 0;
    if (vscroll_mode == 0) {
        a_v_scroll = ((vsram[0] << 8) | vsram[1]) & 0x3FF;
        b_v_scroll = ((vsram[2] << 8) | vsram[3]) & 0x3FF;
    }
    else {
        int x = 1;
    }
    
    if (y < 224) {
        eval_sprites();

        uint8_t vp = reg[0x12] & 0x1F;
        uint8_t hp = reg[0x11] & 0x1F;
        bool in_window = false;
        if (vp) {
            //vertical window set
            int x = 1;
            
            if (reg[0x12] & 0x80) {
                //draw from vp to bottom
                if (line >= (vp * 8)) {
                    in_window = true;
                }
            }
            else {
                //draw from top to vp
                if (line < (vp * 8)) {
                    in_window = true;
                }
            }
        }

        uint16_t hscroll_loc = (reg[0x0D] & 0x3F) << 10;
        uint32_t hscroll_offset = y * 4;
        switch (reg[0x0B] & 0x03) {
            case 0:
                hscroll_offset = 0;
                break;
            case 2:
                hscroll_offset = (y & ~7) * 4;
                break;
            case 3:
                hscroll_offset = y * 4;
                break;
            default: {
                int x = 1;
            } break;
        }
        hscroll_loc += hscroll_offset;


        uint32_t a_y_coarse = a_v_scroll >> 3;
        uint32_t a_y_fine = a_v_scroll & 7;
        uint32_t b_y_coarse = b_v_scroll >> 3;
        uint32_t b_y_fine = b_v_scroll & 7;
        uint32_t a_y_adjusted = y + (a_y_coarse << 3) + a_y_fine;
        uint32_t b_y_adjusted = y + (b_y_coarse << 3) + b_y_fine;
        uint32_t a_y_offset = a_y_adjusted & 7;
        uint32_t b_y_offset = b_y_adjusted & 7;
        uint32_t a_y_address = ((a_y_adjusted >> 3) & (plane_height - 1)) * plane_width * 2;
        uint32_t b_y_address = ((b_y_adjusted >> 3) & (plane_height - 1)) * plane_width * 2;


        uint16_t a_h_scroll = (vram[hscroll_loc + 0] << 8) | vram[hscroll_loc + 1];
        a_h_scroll &= 0x3FF;
        uint16_t b_h_scroll = (vram[hscroll_loc + 2] << 8) | vram[hscroll_loc + 3];
        b_h_scroll &= 0x3FF;

        uint32_t *fb = &frame_buffer[y * 320];
        uint32_t a_x = -((8 - (a_h_scroll & 0x7)) & 7);
        uint32_t b_x = -((8 - (b_h_scroll & 0x7)) & 7);
        uint32_t win_x = 0;

        uint32_t a_nt = (reg[0x02] & 0x38) << 10;
        uint32_t b_nt = (reg[0x04] & 0x7) << 13;

        for (int column = 0; column < 41; column++) {
            uint32_t a_nt_column = (((column * 8) - a_h_scroll) >> 3) & (plane_width - 1);
            uint32_t b_nt_column = (((column * 8) - b_h_scroll) >> 3) & (plane_width - 1);
            
            uint16_t a_nt_address = a_nt + a_y_address + (a_nt_column * 2);
            uint16_t b_nt_address = b_nt + b_y_address + (b_nt_column * 2);

            uint32_t a_tile = (vram[a_nt_address] << 8) | vram[a_nt_address + 1];
            uint32_t b_tile = (vram[b_nt_address] << 8) | vram[b_nt_address + 1];

            uint32_t a_tile_number = a_tile & 0x7FF;
            uint32_t b_tile_number = b_tile & 0x7FF;
            uint8_t a_priority = a_tile >> 15;
            uint8_t b_priority = b_tile >> 15;
            uint32_t a_h_flip = a_tile & 0x800 ? 7 : 0;
            uint32_t b_h_flip = b_tile & 0x800 ? 7 : 0;
            uint32_t a_v_flip = a_tile & 0x1000 ? 7 : 0;
            uint32_t b_v_flip = b_tile & 0x1000 ? 7 : 0;
            uint8_t a_pal = (a_tile >> 13) & 0x3;
            uint8_t b_pal = (b_tile >> 13) & 0x3;
            const uint32_t bytes_per_tile = 32;
            const uint32_t bytes_per_tile_row = bytes_per_tile / 8;
            uint32_t a_pattern_address = a_tile_number * bytes_per_tile + (((a_y_offset & 7) ^ a_v_flip) * bytes_per_tile_row);
            uint32_t b_pattern_address = b_tile_number * bytes_per_tile + (((b_y_offset & 7) ^ b_v_flip) * bytes_per_tile_row);
            uint32_t a_pattern = *((uint32_t *)&vram[a_pattern_address]);
            a_pattern = std::byteswap(a_pattern);
            uint32_t b_pattern = *((uint32_t *)&vram[b_pattern_address]);
            b_pattern = std::byteswap(b_pattern);

            for (int p = 0; p < 8; p++) {
                if (a_x < 320) {

                    uint8_t a_pixel = (a_pattern >> ((7 - (p ^ a_h_flip)) * 4)) & 0xF;
                    a_pixels[a_x] = a_pixel;
                    a_palette[a_x] = a_pal;
                    a_priorities[a_x] = a_priority;
                }
                a_x++;
            }
            for (int p = 0; p < 8; p++) {
                if (b_x < 320) {
                        
                    uint8_t b_pixel = (b_pattern >> ((7 - (p ^ b_h_flip)) * 4)) & 0xF;
                    b_pixels[b_x] = b_pixel;
                    b_palette[b_x] = b_pal;
                    b_priorities[b_x] = b_priority;
                }
                b_x++;
            }

            uint32_t win_nt = (reg[0x03] & 0x3E) << 10;
            //need diff dimensions for H32
            uint32_t win_y_address = ((line >> 3) & (32 - 1)) * 64 * 2;
            uint16_t win_nt_address = win_nt + win_y_address + (column * 2);
            uint32_t win_tile = (vram[win_nt_address] << 8) | vram[win_nt_address + 1];
            uint32_t win_tile_number = win_tile & 0x7FF;
            uint8_t win_priority = win_tile >> 15;
            uint32_t win_h_flip = win_tile & 0x800 ? 7 : 0;
            uint32_t win_v_flip = win_tile & 0x1000 ? 7 : 0;
            uint8_t win_pal = (win_tile >> 13) & 0x3;
            uint32_t win_pattern_address = win_tile_number * bytes_per_tile + (((line & 7) ^ win_v_flip) * bytes_per_tile_row);
            uint32_t win_pattern = std::byteswap(*((uint32_t *)&vram[win_pattern_address]));
                
            for (int p = 0; p < 8; p++) {
                if (win_x < 320) {

                    uint8_t win_pixel = (win_pattern >> ((7 - (p ^ win_h_flip)) * 4)) & 0xF;
                    win_pixels[win_x] = win_pixel;
                    win_palette[win_x] = win_pal;
                    win_priorities[win_x] = win_priority;
                }
                win_x++;
            }

        }
        bool in_h_window = false;

        for (int i = 0; i < 320; i++) {
            uint8_t pixel = reg[0x07] & 0xF;
            uint8_t palette = (reg[0x07] >> 4) & 0x3;

            uint8_t sprite = sprite_buf[i];
            bool sprite_here = sprite != 0xFF;

            if (!in_window && hp) {
                if (reg[0x11] & 0x80) {
                    //from hp to right edge
                    if (i >= (hp * 16)) {
                        in_h_window = true;
                    }
                    else {
                        in_h_window = false;
                    }
                }
                else {
                    //from left edge to hp
                    if (i < (hp * 16)) {
                        in_h_window = true;
                    }
                    else {
                        if (in_h_window && (a_h_scroll & 0xF) && i <= 304) {
                            //emulate window bug
                            //pretty kludgey...
                            for (int j = i; j < i + (a_h_scroll & 0xF); j++) {
                                int src = (j + 16) % 320;
                                a_priorities[j] = a_priorities[src];
                                a_pixels[j] = a_pixels[src];
                                a_palette[j] = a_palette[src];
                            }
                        }
                        in_h_window = false;
                    }
                }
            }

            if (!b_priorities[i] && b_pixels[i]) {
                pixel = b_pixels[i];
                palette = b_palette[i];
            }
            if (in_window || in_h_window) {
                if (!win_priorities[i] && win_pixels[i]) {
                    pixel = win_pixels[i];
                    palette = win_palette[i];
                }
            }
            else {
                if (!a_priorities[i] && a_pixels[i]) {
                    pixel = a_pixels[i];
                    palette = a_palette[i];
                }
            }

            if (sprite_here && !(sprite & 0x40) && (sprite & 0xF) != 0) {
                pixel = sprite & 0xF;
                palette = (sprite >> 4) & 3;
            }

            if (b_priorities[i] && b_pixels[i]) {
                pixel = b_pixels[i];
                palette = b_palette[i];
            }

            if (in_window || in_h_window) {
                if (win_priorities[i] && win_pixels[i]) {
                    pixel = win_pixels[i];
                    palette = win_palette[i];
                }
            }
            else {
                if (a_priorities[i] && a_pixels[i]) {
                    pixel = a_pixels[i];
                    palette = a_palette[i];
                }
            }

            if (sprite_here && sprite & 0x40 && (sprite & 0xF) != 0) {
                pixel = sprite & 0xF;
                palette = (sprite >> 4) & 3;
            }


            if (reg[0x01] & 0x40) {
                *fb = lookup_color(palette, pixel);
            }
            else {
                *fb = 0xFF000000;
            }

            fb++;
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
        hint_counter = reg[0x10];
    }
    else {
        line++;
    }
}

void c_vdp::eval_sprites()
{
    //assumes 320 pixel mode; adjusting for 256 pixel tbd
    //need to change 7e to 7f for 256
    uint16_t sprite_table_base = (reg[0x05] & 0x7E) << 9;
    
    memset(sprite_buf, -1, sizeof(sprite_buf));
    
    const uint32_t sprites_per_frame = 80;
    const uint32_t sprites_per_scanline = 20;
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
        uint32_t vpos = attribute1 & 0x3FF;
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
                    if (h_flip) {
                        int x = 1;
                    }
                    uint32_t hh = (h_flip && hsize) ? hsize - h : h;
                    uint32_t tile = base_tile + vv + (hh * (vsize + 1));
                    uint32_t pattern_address = tile * 32 + ((y_offset ^ v_flip) * 4);
                    uint32_t pattern = std::byteswap(*((uint32_t *)&vram[pattern_address]));
                    int x_start = x + h * 8;
                    for (int j = x_start, p = 0; j < x_start + 8; j++, p++) {
                        if (++pixels_drawn > 320) {
                            return;
                        }
                        if (j >= 0 && j < 320) {
                            if (sprite_buf[j] != 0xFF) {
                                //there is already a sprite here
                                if ((sprite_buf[j] & 0xF) != 0) {
                                    //and it's not transparent
                                    continue;
                                }
                            }
                            uint8_t color = (pattern >> ((7 - (p ^ h_flip)) * 4)) & 0xF;
                            color |= palette;
                            if (priority) {
                                color |= 0x40;
                            }
                            sprite_buf[j] = color;
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