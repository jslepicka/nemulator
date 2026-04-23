module;
#include <cassert>
#include <immintrin.h>
export module tg16:vid;
import nemulator.std;

namespace tg16
{
export template <typename Sys> class c_vid
{
    Sys &sys;

  public:
    c_vid(Sys &sys) : sys(sys)
    {
        reset();
        for (int i = 0; i < 512; i++) {
            uint32_t b = i & 7;
            uint32_t r = (i >> 3) & 7;
            uint32_t g = (i >> 6) & 7;
            r = (r << 5) | (r << 2) | (r >> 1);
            g = (g << 5) | (g << 2) | (g >> 1);
            b = (b << 5) | (b << 2) | (b >> 1);

            palx[i] = (0xFF << 24) | (b << 16) | (g << 8) | r;
        }
    }

    void reset()
    {
        line = 0;
        vblank = 0;
        vce_control = 0;
        vdc_register_latch = 0;
        std::memset(vdc_registers, 0, sizeof(vdc_registers));
        std::memset(satb, 0, sizeof(satb));
        vdc_status = 0;
        raster_compare = 0;
        plane_width = 32;
        plane_height = 32;
        pal_index = 0;
        do_satb_dma = false;
        increment = 1;
    }

    struct s_sprite
    {
        uint8_t pal;
        uint8_t color;
        bool priority;
        bool sprite_here;
    };

    s_sprite sprites[256+16];

    void eval_sprites(int ln)
    {
        uint8_t sprite_pixel_width = (vdc_registers[0x9] >> 2) & 3;
        if (sprite_pixel_width == 3) {
            int x = 1;
        }
        std::memset(sprites, 0, sizeof(sprites));
        for (int i = 0; i < 64; i++) {
            uint16_t word0 = *(uint16_t*)&satb[i * 8 + 0];
            uint16_t word1 = *(uint16_t*)&satb[i * 8 + 2];
            uint16_t word2 = *(uint16_t*)&satb[i * 8 + 4];
            uint16_t word3 = *(uint16_t*)&satb[i * 8 + 6];
            uint16_t vpos = word0 & 0x3FF;
            uint16_t hpos = word1 & 0x3FF;
            uint16_t base_tile = (word2 >> 1) & 0x3FF;
            uint16_t hsize = (word3 >> 8) & 0x1;
            uint16_t vsize = (word3 >> 12) & 0x3;
            if (vsize > 1) {
                vsize = 2;
            }
            uint16_t palette = word3 & 0xF;
            bool priority = word3 & 0x80;
            uint32_t v_flip = word3 & 0x8000 ? 15 : 0;
            uint32_t h_flip = word3 & 0x800 ? 15 : 0;

            int32_t y = (int32_t)vpos - 64;
            int32_t x = (int32_t)hpos - 32;

            for (int v = 0; v < vsize + 1; v++) {
                int32_t y_base = y + v * 16;
                if (ln >= y_base && ln < y_base + 16) {
                    int32_t y_offset = ln - y_base;
                    uint32_t vv = (v_flip && vsize) ? vsize - v : v;

                    for (int h = 0; h < hsize + 1; h++) {
                        uint32_t hh = (h_flip && hsize) ? hsize - h : h;
                        uint32_t tile = base_tile + hh + (vv * (hsize + 1));
                        uint32_t pattern_address = tile * 128 + ((y_offset ^ v_flip) * 2);

                        uint32_t p0 = *(uint16_t*)&vram[pattern_address];
                        uint32_t p1 = *(uint16_t *)&vram[pattern_address + 32];
                        uint32_t p2 = *(uint16_t *)&vram[pattern_address + 64];
                        uint32_t p3 = *(uint16_t *)&vram[pattern_address + 96];

                        if (sprite_pixel_width == 3) {
                            if (word2 & 0x1) {
                                p0 = p2;
                                p1 = p3;
                            }
                            p2 = 0;
                            p3 = 0;
                        }

                        p0 <<= 0;
                        p1 <<= 1;
                        p2 <<= 2;
                        p3 <<= 3;

                        int x_start = x + h * 16;
                        for (int j = x_start, p = 0; j < x_start + 16; j++, p++) {
                            if (j >= 0 && j < 256) {
                                if (sprites[j].sprite_here && sprites[j].color) {
                                    continue;
                                }
                                uint8_t pp = p ^ h_flip;
                                uint8_t px = ((p0 >> (15 - pp)) & 1) |
                                             ((p1 >> (15 - pp)) & 2) |
                                             ((p2 >> (15 - pp)) & 4) |
                                             ((p3 >> (15 - pp)) & 8);

                                sprites[j].sprite_here = true;
                                sprites[j].color = px;
                                sprites[j].pal = palette;
                            }
                            //p0 >>= (7-i);
                            //p1 >>= (7-i);
                            //p2 >>= (7-i);
                            //p3 >>= (7-i);
                        }
                    }
                }
            }
        }
    }

    void do_scanline()
    {
        line++;

        uint8_t VSW = vdc_registers[0x0C] & 0x1F;
        uint8_t VDS = vdc_registers[0x0C] >> 8;
        uint32_t y_scroll = vdc_registers[0x08];
        uint32_t x_scroll = vdc_registers[0x07];

        if (x_scroll) {
            int x = 1;
        }

        uint32_t start_line = VSW + VDS;
        if (line >= start_line && line < start_line + vdc_registers[0x0D]) {
            eval_sprites(line - start_line);
            uint32_t temp[256 + 8] = {0};
            if (!(vdc_registers[0x5] & 0x80)) {
                std::fill_n(&fb[(line - start_line) * 256], 256, 0xFF000000);
            }
            else {
                uint32_t y = line - start_line + y_scroll;

                uint32_t x = 0;
                for (int column = 0; column < display_width + 1; column++) {
                    uint32_t y_address = ((y >> 3) & (plane_height - 1)) * plane_width * 2;
                    uint32_t nt_column = (((column * 8) + x_scroll) >> 3) & (plane_width - 1);
                    uint32_t nt_address = y_address + (nt_column * 2);
                    uint32_t tile = *((uint16_t *)&vram[nt_address]);
                    uint32_t palette = tile >> 12;
                    tile &= 0xFFF;
                    uint32_t tile_address = tile * 32;
                    tile_address += (y & 7) * 2;

                    uint64_t pdep_pattern = 0b00000001'00000001'00000001'00000001'00000001'00000001'00000001'00000001;
                    uint64_t c = _pdep_u64(vram[tile_address], pdep_pattern);
                    c |= _pdep_u64(vram[tile_address + 1], pdep_pattern << 1);
                    c |= _pdep_u64(vram[tile_address + 16], pdep_pattern << 2);
                    c |= _pdep_u64(vram[tile_address + 17], pdep_pattern << 3);

                    c = std::byteswap(c);

                    uint16_t pal_index = palette * 16;

                    for (int i = 0; i < 8; i++) {
                        uint32_t color;
                        if (c & 0xF) {
                            color = palx[pal[pal_index + (c & 0xF)]];
                        }
                        else {
                            color = palx[pal[0]];
                        }
                        temp[x] = color;
                        c >>= 8;
                        x++;
                    }
                }
            }

            for (int i = 0; i < 256; i++) {
                if (sprites[i].sprite_here && sprites[i].color) {
                    temp[(x_scroll & 7) + i] = palx[pal[256 + (sprites[i].color | (sprites[i].pal << 4))]];
                }
            }

            std::memcpy(&fb[(line - start_line) * 256], &temp[x_scroll & 0x7], 256 * sizeof(uint32_t));
        }

        uint32_t RCR = vdc_registers[0x06] & 0x3FF;

        int x = (int)RCR - 0x40 + (int)start_line;

        if (line == x) {
            raster_compare = 1;
            if (vdc_registers[0x05] & 0x4) {
                sys.irq1 = 1;
                vdc_status |= 0x4;
                //std::printf("rcr irq\n");
            }
        }

        if (line == 257) {
            //vblank
            vblank = 1;
            if (vdc_registers[0x05] & 0x8) {
                sys.irq1 = 1;
                vdc_status |= 0x20;
                //std::printf("vblank irq\n");
            }
        }
        else if (line == 258) {
            // todo: proper timing of satb transfer
            if ((vdc_registers[0xF] & 0x10) || do_satb_dma) {
                do_satb_dma = false;
                uint32_t src = vdc_registers[0x13] * 2;
                for (int i = 0; i < 512; i++) {
                    satb[i] = vram[(src + i) & 0xFFFF];
                }
                if (vdc_registers[0xF] & 0x1) {
                    vdc_status |= 0x8;
                    sys.irq1 = 1;
                }
            }
        }
        else if (line == 263) {
            line = 0;
        }
    }

    void write_vdc(uint16_t address, uint8_t value)
    {
        int x = 1;
        switch (address & 0x3) {
            case 0:
                vdc_register_latch = value;
                break;
            case 1:
                //assert(0);
                break;
            case 2:
                write_vdc_register_lo(value);
                break;
            case 3:
                write_vdc_register_hi(value);
                break;
        }
    }

    uint8_t read_vdc(uint16_t address)
    {
        int x = 1;
        uint8_t ret = 0;
        switch (address & 0x3) {
            case 0:
                ret = vdc_status;
                vdc_status &= ~0x3F;
                sys.irq1 = 0;
                raster_compare = 0;
                vblank = 0;
                return ret;
            case 2:
                return read_vdc_register_lo();
            case 3:
                return read_vdc_register_hi();
        }
        return 0;
    }

    uint8_t read_vdc_register_lo()
    {
        switch (vdc_register_latch) {
            case 0x2:
                return read_buffer & 0xFF;
            default:
                return 0xCD;

        }
        return 0;
    }

    uint8_t read_vdc_register_hi()
    {
        uint8_t ret;
        switch (vdc_register_latch) {
            case 0x2:
                ret = read_buffer >> 8;
                vdc_registers[0x1] += increment;
                read_buffer = *(uint16_t *)&vram[vdc_registers[0x01] * 2];
                return ret;

            default:
                return 0xCD;
        }
        return 0;
    }

    void write_vdc_register_lo(uint8_t value)
    {
        int x = 1;
        if (vdc_register_latch == 0x0 && value != 0) {
            int x = 1;
        }
        vdc_registers[vdc_register_latch] = (vdc_registers[vdc_register_latch] & 0xFF00) | value;
        switch (vdc_register_latch) {
            case 0x05: {
                uint8_t prev = vdc_registers[0x5];
                if (value ^ prev && value & 0x8) {
                    if (vblank) {
                        sys.irq1 = 1;
                        vdc_status |= 0x20;
                        std::printf("delayed vblank irq\n");
                    }
                }
                if (value ^ prev && value & 0x4) {
                    if (raster_compare) {
                        int x = 1;
                    }
                }
            } break;
            case 0x9:
                plane_height = value & 0x40 ? 64 : 32;
                switch ((value & 0x30) >> 4) {
                    case 0:
                        plane_width = 32;
                        break;
                    case 1:
                        plane_width = 64;
                        break;
                    case 2:
                    case 3:
                        plane_width = 128;
                        break;
                }
                break;
            case 0x0B:
                display_width = vdc_registers[0x0B] & 0x3F;
                display_width += 1;
                break;

            case 0x0D:
                display_height = vdc_registers[0x0D] & 0x1FF;
                display_height += 1;
                break;
            case 0x0F:
                std::printf("write %02X to DMA control register\n", value);
                break;
            case 0x10:
                std::printf("write %02X to DMA source address register\n", value);
                break;
            case 0x11:
                std::printf("write %02X to DMA dest address register\n", value);
                break;
            case 0x12:
                std::printf("write %02X to DMA block length register\n", value);
                break;
            case 0x13:
                std::printf("write %02X to DMA VRAM-SATB source\n", value);
                //do_satb_dma = true;
                break;
        }
    }
    
    void write_vdc_register_hi(uint8_t value)
    {
        int x = 1;
        if (vdc_register_latch == 0x0 && value != 0) {
            int x = 1;
        }
        vdc_registers[vdc_register_latch] = (vdc_registers[vdc_register_latch] & 0x00FF) | (value << 8);
        switch (vdc_register_latch) {
            case 0x00:
                // MAWR
                x = 2;
                break;
            case 0x01:
                // MARR
                x = 2;
                read_buffer = *(uint16_t*)&vram[vdc_registers[0x01] * 2];
                break;
            case 0x02: {
                uint16_t &MAWR = vdc_registers[0];
                uint32_t offset = MAWR * 2;
                offset &= 0xFFFF;
                *(uint16_t *)&vram[offset] = vdc_registers[2];
                MAWR += increment;
            }
                break;
            case 0x05:
                switch ((value >> 11) & 0x3) {
                    case 0:
                        increment = 1;
                        break;
                    case 1:
                        increment = 0x20;
                        break;
                    case 2:
                        increment = 0x40;
                        break;
                    case 3:
                        increment = 0x80;
                        break;
                }
                break;
            case 0x0D:
                display_height = vdc_registers[0x0D] & 0x1FF;
                display_height += 1;
                break;
            case 0x0F:
                std::printf("write %02X to DMA control register hi\n", value);
                break;
            case 0x10:
                std::printf("write %02X to DMA source address register hi\n", value);
                break;
            case 0x11:
                std::printf("write %02X to DMA dest address register hi\n", value);
                break;
            case 0x12:
                std::printf("write %02X to DMA block length register hi\n", value);
                break;
            case 0x13:
                std::printf("write %02X to DMA VRAM-SATB source hi\n", value);
                do_satb_dma = true;
                break;
            default:
                x = 2;
                break;
                
        }
    }

    void write_vce(uint16_t address, uint8_t value)
    {
        int x = 1;
        switch (address & 0x7) {
            case 0:
                vce_control = value;
                break;
            case 2:
                pal_index = (pal_index & 0x100) | value;
                break;
            case 3:
                pal_index = (pal_index & 0xFF) | ((value & 0x1) << 8);
                break;
            case 4:
                pal[pal_index] = (pal[pal_index] & 0x100) | value;
                break;
            case 5:
                pal[pal_index] = (pal[pal_index] & 0xFF) | ((value & 0x1) << 8);
                pal_index++;
                pal_index &= 0x1FF;
                break;
        }
    }

  public:
    int line;
    uint8_t vdc_status;
    uint32_t fb[256 * 256];
  private:
    
    int vblank;
    int raster_compare;

    uint16_t read_buffer;

    uint8_t vdc_register_latch;
    uint8_t vdc_data_lo;
    uint8_t vdc_data_hi;
    uint16_t vdc_registers[20];
    
    uint8_t vram[65536];
    uint32_t plane_width;
    uint32_t plane_height;
    uint32_t display_width;
    uint32_t display_height;

    uint8_t vce_control;
    uint16_t pal[512];
    uint16_t pal_index;
    uint32_t palx[512];

    bool do_satb_dma;
    uint8_t satb[512];

    int increment;

};
} //namespace tg16