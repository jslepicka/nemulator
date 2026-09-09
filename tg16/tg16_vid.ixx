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

            rgb[i] = (0xFF << 24) | (b << 16) | (g << 8) | r;
        }
    }

    void reset()
    {
        vce_line = 0;
        vce_lines = 262;
        vphase = VPHASE_VSW;
        vphase_count = 1;
        rcr_counter = 0x40;
        vblank = 0;
        vce_control = 0;
        vdc_register_latch = 0;
        std::memset(vdc_registers, 0, sizeof(vdc_registers));
        std::memset(satb, 0, sizeof(satb));
        std::memset(pal, 0, sizeof(pal));
        vdc_status = 0;
        raster_compare = 0;
        read_buffer = 0;
        plane_width = 32;
        plane_height = 32;
        display_width = 32;
        display_height = 240;
        pal_index = 0;
        do_satb_dma = false;
        increment = 1;
        VSW = 0;
        VDS = 0;
        VDW = 0;
        VCR = 0;
        y_offset = 0;
        reload_y_scroll = false;
        std::fill_n(fb, 256 * 256, 0xFF000000);
        burst_mode = false;
    }

    struct s_sprite
    {
        uint8_t color;
        bool priority;
    };

    s_sprite sprite_output[256];

    void eval_sprites(int ln)
    {
        uint8_t sprite_pixel_width = (vdc_registers[0x9] >> 2) & 3;
        if (sprite_pixel_width == 3) {
            int x = 1;
        }
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
            if (vsize == 3) {
                vsize = 2;
            }

            constexpr uint16_t base_tile_mask[4] = {~0, ~2, ~6, ~6};
            base_tile &= base_tile_mask[vsize] & ~hsize;

            uint16_t palette = (word3 & 0xF) << 4;
            bool priority = word3 & 0x80;
            bool h_flip = word3 & 0x800;
            bool v_flip = word3 & 0x8000;
            uint32_t h_tile_flip = h_flip ? hsize : 0;
            uint32_t v_tile_flip = v_flip ? (1 << vsize) - 1 : 0;
            uint32_t v_offset_flip = v_flip ? 15 : 0;

            int32_t y = (int32_t)vpos - 64;
            int32_t x = (int32_t)hpos - 32;

            int32_t sprite_height = 16 << vsize;

            if (ln >= y && ln < y + sprite_height) {
                uint32_t y_offset = ln - y;
                uint32_t v = (y_offset >> 4) ^ v_tile_flip;
            
                for (int h = 0; h < hsize + 1; h++) {
                    uint32_t tile = base_tile | (v << 1) | (h ^ h_tile_flip);
                    uint32_t pattern_address = tile * 128 + (((y_offset & 0xF) ^ v_offset_flip) * 2);

                    uint16_t p0 = 0;
                    uint16_t p1 = 0;
                    uint16_t p2 = 0;
                    uint16_t p3 = 0;

                    if (sprite_pixel_width == 3) {
                        uint32_t pattern_offset = (word2 & 0x1) * 64;
                        p0 = *(uint16_t *)&vram[pattern_address + pattern_offset];
                        p1 = *(uint16_t *)&vram[pattern_address + pattern_offset + 32];
                    }
                    else {
                        p0 = *(uint16_t *)&vram[pattern_address];
                        p1 = *(uint16_t *)&vram[pattern_address + 32];
                        p2 = *(uint16_t *)&vram[pattern_address + 64];
                        p3 = *(uint16_t *)&vram[pattern_address + 96];
                    }

                    constexpr uint64_t pdep_pattern = broadcast8to64(0x11);
                    uint64_t c = _pdep_u64(p0, pdep_pattern);
                    c |= _pdep_u64(p1, pdep_pattern << 1);
                    c |= _pdep_u64(p2, pdep_pattern << 2);
                    c |= _pdep_u64(p3, pdep_pattern << 3);

                    if (!h_flip) {
                        c = std::byteswap(c);
                        c = ((c & broadcast8to64(0x0F)) << 4) | ((c & broadcast8to64(0xF0)) >> 4);
                    }

                    int x_start = x + h * 16;
                    for (int j = x_start; j < x_start + 16; j++, c >>= 4) {
                        if (j >= 0 && j < 256) {
                            if (sprite_output[j].color) {
                                continue;
                            }
                            uint8_t pal_index = c & 0xF;
                            if (pal_index) {
                                sprite_output[j].color = pal_index | palette;
                                sprite_output[j].priority = priority;
                            }
                        }
                    }
                }
            }
        }
    }

    bool burst_mode;

    void render_display_line(uint32_t *pfb, int display_row)
    {
        if (reload_y_scroll) {
            reload_y_scroll = false;
            y_offset = vdc_registers[0x08];
        }
        uint32_t y = y_offset++;

        if (!pfb) {
            //this display line falls outside the visible framebuffer
            return;
        }

        if (burst_mode) {
            std::fill_n(pfb, 256, rgb[pal[256]]);
            return;
        }

        std::memset(sprite_output, 0, sizeof(sprite_output));
        if (vdc_registers[0x5] & 0x40) {
            eval_sprites(display_row);
        }

        uint32_t x_scroll = vdc_registers[0x07];
        uint8_t temp[(64*8) + 8] = {0};
        if (vdc_registers[0x5] & 0x80) {
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

                constexpr uint64_t pdep_pattern = broadcast8to64(0x01);
                uint64_t c = _pdep_u64(vram[tile_address], pdep_pattern);
                c |= _pdep_u64(vram[tile_address + 1], pdep_pattern << 1);
                c |= _pdep_u64(vram[tile_address + 16], pdep_pattern << 2);
                c |= _pdep_u64(vram[tile_address + 17], pdep_pattern << 3);

                c = std::byteswap(c);

                // only broadcast palette where the pattern is not 0.
                // allows for simpler check when merging in sprites later
                // as we don't have to mask out the palette/store it separately.
                uint64_t broadcast_mask = c | (c >> 1);
                broadcast_mask |= (broadcast_mask >> 2);
                broadcast_mask &= 0x0101010101010101;

                uint64_t pal_broadcast = (palette << 4) * broadcast_mask;
                c |= pal_broadcast;

                *(uint64_t *)&temp[x] = c;
                x += 8;
            }
        }

        uint8_t *pbg = &temp[x_scroll & 0x7];
        for (int i = 0; i < 256; i++) {
            uint32_t pal_index = *pbg++;
            if (sprite_output[i].color && (sprite_output[i].priority || !pal_index)) {
                pal_index = 256 + sprite_output[i].color;
            }
            *pfb++ = rgb[pal[pal_index]];
        }
    }

    // the vdc vertical state machine runs VSW -> VDS -> VDW -> VCR.  it advances on
    // its own line count rather than absolute line numbers, and each phase latches
    // its own register at the moment it starts.  the vce's vsync restarts it at the
    // frame boundary (see do_scanline).
    void next_vphase()
    {
        switch (vphase) {
            case VPHASE_VSW:
                vphase = VPHASE_VDS;
                VDS = vdc_registers[0x0C] >> 8;
                vphase_count = VDS + 2;
                break;

            case VPHASE_VDS: {
                vphase = VPHASE_VDW;
                VDW = vdc_registers[0x0D] & 0x1FF;
                vphase_count = VDW + 1;
                // the raster counter is reloaded at the start of the display period.
                // seeded one low: the increment at the top of the next call -- the
                // call that draws the first display line -- brings it to $40.
                rcr_counter = 0x40 - 1;
                bool prev_burst = burst_mode;
                burst_mode = !(vdc_registers[0x5] & 0xC0);
                if (burst_mode && !prev_burst) {
                    ods("-- burst mode --\n");
                }
                reload_y_scroll = true;
            } break;

            case VPHASE_VDW:
                vphase = VPHASE_VCR;
                VCR = vdc_registers[0x0E] & 0xFF;
                vphase_count = VCR;
                vblank = 1;
                if (vdc_registers[0x05] & 0x8) {
                    sys.irq1 = 1;
                    vdc_status |= 0x20;
                    //ods("vblank irq\n");
                }
                // satb transfer starts at the beginning of vertical blanking
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
                break;

            case VPHASE_VCR:
                vphase = VPHASE_VSW;
                VSW = vdc_registers[0x0C] & 0x1F;
                vphase_count = VSW + 1;
                break;
        }
    }

    // called at the start of each scanline, before the cpu runs the line.
    // rcr_counter names the line about to be drawn: it is $40 on the first display
    // line, so RCR = $40 + k raises the interrupt at the start of display line k.
    // do_scanline() draws that line LATCH_DOT dots later, and that render point
    // doubles as the scroll register latch: a handler quick enough to write
    // BXR/BYR before it moves line k itself, a slower one only lands on line k + 1.
    // measured from the raster irq to the scroll write: Vigilante 56 dots (one
    // BXR write, splits on line k), Bloody Wolf 269 dots (BYR + BXR, splits on
    // line k + 1).  LATCH_DOT sits between the two.
    void line_start()
    {
        rcr_counter++;
        if (rcr_counter == (vdc_registers[0x06] & 0x3FF)) {
            raster_compare = 1;
            if (vdc_registers[0x05] & 0x4) {
                sys.irq1 = 1;
                vdc_status |= 0x4;
                //ods("rcr irq at line %d\n", vce_line);
            }
        }
    }

    // draws the current line, then advances the vdc phase and the vce raster.
    // returns true when the vce raster wraps, i.e. the frame is complete
    bool do_scanline()
    {
        int fb_row = vce_line - fb_top_line;
        uint32_t *pfb = (fb_row >= 0 && fb_row < 240) ? &fb[fb_row * 256] : nullptr;

        if (vphase == VPHASE_VDW) {
            render_display_line(pfb, VDW + 1 - vphase_count);
        }
        else if (pfb) {
            std::fill_n(pfb, 256, rgb[pal[256]]);
        }

        if (--vphase_count <= 0) {
            // VCR can legitimately be zero, so a phase may have no lines at all
            int guard = 4;
            do {
                next_vphase();
            } while (vphase_count <= 0 && --guard);
            if (vphase_count <= 0) {
                vphase_count = 1;
            }
        }

        if (++vce_line >= vce_lines) {
            vce_line = 0;
            vce_lines = (vce_control & 0x4) ? 263 : 262;
            // the vce is the sync master.  vsync restarts the vdc's vertical state
            // machine, so a vdc programmed for more lines than the vce provides just
            // gets its trailing VCR phase truncated instead of rolling the picture.
            // Cadash asks for 264 lines (VCR=4) against a 263 line vce; Vigilante
            // asks for exactly 263 (VCR=3) and this resync is a no-op for it.
            vphase = VPHASE_VSW;
            VSW = vdc_registers[0x0C] & 0x1F;
            vphase_count = VSW + 1;
            return true;
        }
        return false;
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
            case 0x2: {
                ret = read_buffer >> 8;
                uint16_t &MARR = vdc_registers[0x01];
                MARR += increment;
                if (MARR > 0x7FFF) {
                    //assert(0);
                    ods("Out of bounds VRAM read (%04X)\n", MARR);
                }
                read_buffer = *(uint16_t *)&vram[MARR * 2];
                return ret;
            }

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
        uint16_t prev = vdc_registers[vdc_register_latch];
        uint16_t &r = vdc_registers[vdc_register_latch];
        r = (r & 0xFF00) | value;
        switch (vdc_register_latch) {
            case 0x02:
                x = 2;
                break;
            case 0x05: {
                
                if (r ^ prev && r & 0x8) {
                    if (vblank) {
                        sys.irq1 = 1;
                        vdc_status |= 0x20;
                        ods("delayed vblank irq\n");
                    }
                }
                if (r ^ prev && r & 0x4) {
                    if (raster_compare) {
                        int x = 1;
                        //assert(0);
                    }
                }

                if ((r ^ prev) & 0x80) {
                    if (r & 0x80) {
                        ods("bg rendering enabled at line %d\n", vce_line);
                    }
                    else {
                        ods("bg rendering disabled at line %d\n", vce_line);
                    }
                }

            } break;
            case 0x06:
                ods("set rcr lo to %04X (%d) at line %d\n", r, r, vce_line);
                break;
            case 0x8:
                reload_y_scroll = true;
                //ods("set y scroll to %d at line %d\n", r, vce_line);
                break;
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
                ods("set display width to %d\n", display_width);
                break;

            case 0x0D:
                display_height = vdc_registers[0x0D] & 0x1FF;
                display_height += 1;
                ods("set display height to %d\n", display_height);
                break;
            case 0x0F:
                ods("write %02X to DMA control register\n", value);
                break;
            case 0x10:
                ods("write %02X to DMA source address register\n", value);
                break;
            case 0x11:
                ods("write %02X to DMA dest address register\n", value);
                break;
            case 0x12:
                ods("write %02X to DMA block length register\n", value);
                break;
            case 0x13:
                //ods("write %02X to DMA VRAM-SATB source\n", value);
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
        uint16_t &r = vdc_registers[vdc_register_latch];
        r = (r & 0x00FF) | (value << 8);
        switch (vdc_register_latch) {
            case 0x00:
                // MAWR
                x = 2;
                break;
            case 0x01:
                // MARR
                x = 2;
                if (r > 0x7FFF) {
                    assert(0);
                }
                read_buffer = *(uint16_t*)&vram[r * 2];
                break;
            case 0x02: {
                uint16_t &MAWR = vdc_registers[0];
                uint32_t offset = MAWR * 2;
                offset &= 0xFFFF;
                if (MAWR < 0x8000) {
                    //writes past 64k are ignored
                    //fixes graphics corruption in Vigilante attract mode bridge scene
                    *(uint16_t *)&vram[offset] = r;
                }
                MAWR += increment;
            }
                break;
            case 0x05:
                // IW is bits 11-12 of the 16 bit control register, so it has to be
                // taken from the whole register - shifting the high byte by 11
                // always yielded 0, pinning the increment at 1.
                switch ((r >> 11) & 0x3) {
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
            case 0x06:
                ods("set rcr lo to %04X (%d) at line %d\n", r, r, vce_line);
                break;
            case 0x07:
                r &= 0x3FF;
                break;
            case 0x08:
                r &= 0x1FF;
                //ods("set y scroll to %d at line %d\n", r, vce_line);
                reload_y_scroll = true;
                break;
            case 0x0D:
                display_height = r & 0x1FF;
                display_height += 1;
                ods("set display height to %d\n", display_height);
                break;
            case 0x0F:
                ods("write %02X to DMA control register hi\n", value);
                break;
            case 0x10:
                ods("write %02X to DMA source address register hi\n", value);
                break;
            case 0x11:
                ods("write %02X to DMA dest address register hi\n", value);
                break;
            case 0x12:
                ods("write %02X to DMA block length register hi\n", value);
                break;
            case 0x13:
                //ods("write %02X to DMA VRAM-SATB source hi\n", value);
                do_satb_dma = true;
                break;
            default:
                x = 2;
                break;
                
        }
    }

    uint8_t read_vce(uint16_t address)
    {
        uint8_t ret = 0;
        switch (address & 0x7) {
            case 4:
                return pal[pal_index] & 0xFF;
            case 5:
                ret = pal[pal_index] >> 8;
                pal_index++;
                pal_index &= 0x1FF;
                return ret;
            default:
                return 0;
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
    int vce_line;
    uint8_t vdc_status;
    uint32_t fb[256 * 256];
  private:
    // vce raster: defines the frame.  262 or 263 lines, from vce control bit 2.
    int vce_lines;
    // vce line that maps to framebuffer row 0.  this is the display start for the
    // standard VSW=2/VDS=15 timing ((2+1) + (15+2)), so normal games land where
    // they always have.
    static constexpr int fb_top_line = 20;

    // vdc vertical state machine: free-runs, independent of the vce raster
    enum e_vphase { VPHASE_VSW, VPHASE_VDS, VPHASE_VDW, VPHASE_VCR };
    int vphase;
    int vphase_count;
    // internal raster counter for RCR.  reloaded to 0x40 at the start of the
    // display period and then counts continuously, blanking included.
    int rcr_counter;

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
    uint32_t rgb[512];

    bool do_satb_dma;
    uint8_t satb[512];

    int increment;

    uint8_t VSW;
    uint8_t VDS;
    uint8_t VCR;
    uint16_t VDW;
    uint32_t y_offset;
    bool reload_y_scroll;

};
} //namespace tg16