module;
#include <cassert>
#include <immintrin.h>
export module tg16:vid;
import nemulator.std;

namespace tg16
{

export template <typename Sys> class c_vid
{
    typedef std::function<void(int)> mode_switch_callback_t;

    Sys &sys;
    mode_switch_callback_t mode_switch_callback;

  public:
    // 512 is the widest mode any commercial game uses.  the hardware could show a
    // little more - a scanline is 1365 master clocks, so 682.5 dots at the fastest
    // dot clock (master / 2), of which roughly 565 fall inside the ntsc active line
    // time - and HDW is 7 bits, so it can be programmed wider still.  anything past
    // this is clamped rather than rejected.
    static constexpr int max_width = 512;

    c_vid(Sys &sys, mode_switch_callback_t mode_switch_callback)
        : sys(sys), mode_switch_callback(mode_switch_callback)
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
        frame_complete = false;
        ph_hds = ph_hdw = ph_hde = ph_hsw = 341;
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
        frame_width = 0;
        frame_x = 0;
        pal_index = 0;
        do_satb_dma = false;
        increment = 1;
        VSW = 0;
        VDS = 0;
        VDW = 0;
        VCR = 0;
        y_offset = 0;
        reload_y_scroll = false;
        byr_written = false;
        std::fill_n(fb, max_width * 240, 0xFF000000);
        burst_mode = false;
    }

    struct s_sprite
    {
        uint8_t color;
        bool priority;
    };

    s_sprite sprite_output[max_width];

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
                        if (j >= 0 && j < frame_width) {
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

    // the active width only changes between frames.  latching it at the start of
    // the display period and telling the system about it lets the front end re-crop,
    // the same way the genesis vdp reports a 256/320 switch.
    void update_width()
    {
        int w = display_width * 8;
        if (w > max_width) {
            w = max_width;
        }
        if (w < 8) {
            w = 8;
        }
        if (w != frame_width) {
            frame_width = w;
            frame_x = (max_width - frame_width) / 2;
            mode_switch_callback(frame_width);
        }
    }

    void render_display_line(uint32_t *pfb, int display_row)
    {
        if (reload_y_scroll) {
            //start of the display period: the counter takes BYR and the first
            //line shows it exactly as written
            reload_y_scroll = false;
            byr_written = false;
            y_offset = vdc_registers[0x08];
        }
        else if (byr_written) {
            //a write part way down the frame lands after the counter has already
            //stepped for the next line, so that line shows BYR + 1
            byr_written = false;
            y_offset = vdc_registers[0x08] + 1;
        }
        uint32_t y = y_offset++;

        if (!pfb) {
            //this display line falls outside the visible framebuffer
            return;
        }

        if (burst_mode) {
            std::fill_n(pfb, frame_width, rgb[pal[256]]);
            return;
        }

        std::memset(sprite_output, 0, frame_width * sizeof(sprite_output[0]));
        if (vdc_registers[0x5] & 0x40) {
            eval_sprites(display_row);
        }

        uint32_t x_scroll = vdc_registers[0x07];
        uint8_t temp[max_width + 8] = {0};
        if (vdc_registers[0x5] & 0x80) {
            uint32_t x = 0;
            for (int column = 0; column < (frame_width / 8) + 1; column++) {
                uint32_t y_address = ((y >> 3) & (plane_height - 1)) * plane_width * 2;
                uint32_t nt_column = (((column * 8) + x_scroll) >> 3) & (plane_width - 1);
                uint32_t nt_address = y_address + (nt_column * 2);
                uint32_t tile = *((uint16_t *)&vram[nt_address]);
                uint32_t palette = tile >> 12;
                tile &= 0xFFF;
                uint32_t tile_address = tile * 32;
                tile_address += (y & 7) * 2;
                tile_address &= 0xFFFF;

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
        for (int i = 0; i < frame_width; i++) {
            uint32_t pal_index = *pbg++;
            if (sprite_output[i].color && (sprite_output[i].priority || !pal_index)) {
                pal_index = 256 + sprite_output[i].color;
            }
            *pfb++ = rgb[pal[pal_index]];
        }
    }

    // vram-to-vram block copy, started by writing the high byte of LENR.
    // DCR bit 2 counts the source down instead of up, bit 3 does the same for the
    // destination, and bit 1 enables the completion interrupt.  the real vdc steals
    // cycles from the cpu for this; we do it instantly, like the satb transfer.
    void do_vram_dma()
    {
        uint16_t src = vdc_registers[0x10];
        uint16_t dst = vdc_registers[0x11];
        uint32_t len = vdc_registers[0x12];
        int src_step = (vdc_registers[0x0F] & 0x04) ? -1 : 1;
        int dst_step = (vdc_registers[0x0F] & 0x08) ? -1 : 1;

        do {
            if (dst < 0x8000) {
                *(uint16_t *)&vram[dst * 2] = *(uint16_t *)&vram[(src & 0x7FFF) * 2];
            }
            src = (uint16_t)(src + src_step);
            dst = (uint16_t)(dst + dst_step);
        } while (len--);

        vdc_registers[0x10] = src;
        vdc_registers[0x11] = dst;
        vdc_registers[0x12] = 0xFFFF;

        if (vdc_registers[0x0F] & 0x02) {
            vdc_status |= 0x10;
            sys.irq1 = 1;
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
                reload_y_scroll = true;
                update_width();
            } break;

            case VPHASE_VDW:
                vphase = VPHASE_VCR;
                VCR = vdc_registers[0x0E] & 0xFF;
                vphase_count = VCR;
                //entering vertical blanking only arms the interrupt; it is raised
                //at the hds point of the following line, which is what keeps it
                //clear of the raster compare that flagged it
                vblank_pending = true;
                break;

            case VPHASE_VCR:
                vphase = VPHASE_VSW;
                VSW = vdc_registers[0x0C] & 0x1F;
                vphase_count = VSW + 1;
                // burst mode is sampled entering vsync, not at the start of the
                // display period.  latching it here stops a background enabled
                // during vertical blanking from appearing a frame early - Bonk's
                // Adventure flashes its title screen before it scrolls in.
                burst_mode = !(vdc_registers[0x5] & 0xC0);
                break;
        }
    }

    // ---------------------------------------------------------------------
    // the line is driven as the vdc's four horizontal phases, HDS -> HDW ->
    // HDE -> HSW, with the cpu run in between.  that puts each hardware event
    // where the vdc actually performs it: the scroll registers are latched
    // entering HDS, and the vertical state advances - taking the vblank
    // interrupt and the satb transfer with it - during HSW.
    // ---------------------------------------------------------------------

    // work out this line's phase lengths.  the vdc's programmed line rarely adds
    // up to the 1365 master clocks a scanline really takes, so the sync phase
    // absorbs the difference the way the incoming hsync does on hardware.
    void compute_line_timing()
    {
        static constexpr int dot_master[4] = {4, 3, 2, 2};   //5.37 / 7.16 / 10.74MHz
        int dm = dot_master[vce_control & 3];
        dot_clock = dm;

        ph_hds = (((vdc_registers[0x0A] >> 8) & 0x7F) + 1) * 8 * dm;
        ph_hdw = ((vdc_registers[0x0B] & 0x7F) + 1) * 8 * dm;
        ph_hde = (((vdc_registers[0x0B] >> 8) & 0x7F) + 1) * 8 * dm;

        //trim from the back until the line fits, then give the remainder to hsw
        if (ph_hds + ph_hdw + ph_hde > line_master - 8) {
            ph_hde = std::max(8, line_master - 8 - ph_hds - ph_hdw);
            if (ph_hds + ph_hdw + ph_hde > line_master - 8) {
                ph_hdw = std::max(8, line_master - 8 - ph_hds - ph_hde);
                if (ph_hds + ph_hdw + ph_hde > line_master - 8) {
                    ph_hds = std::max(8, line_master - 8 - ph_hdw - ph_hde);
                }
            }
        }
        ph_hsw = line_master - ph_hds - ph_hdw - ph_hde;
    }

    // a scanline is driven as three events, placed where the vdc performs them.
    // between them the cpu runs for the real interval, so a handler either reaches
    // the scroll registers before the row is fetched or it does not - which is the
    // distinction the hardware makes, and the only one a raster split depends on.
    //
    //    -34 dots   scroll latch, row fetched        phase_latch
    //    -26 dots   vertical blank irq, satb dma     phase_hds_irq
    //      0 dots   display window opens
    //   +HDW-14     raster compare, vertical advance phase_rcr
    //
    // the offsets are counted from the display window opening.  the latch sitting
    // 34 dots ahead of it puts it back inside the preceding sync period for the
    // usual HDS, and that is what decides a split: Splatterhouse writes its bottom
    // split too late to catch the latch, which leaves the black line hardware shows
    // above the status bar, while Vigilante and Cadash's scroll chain get there in
    // time and move the row they named.

    // the vdc latches the scroll registers and fetches the row here.  whatever the
    // cpu wrote before this instant is what the line is drawn with.
    int phase_latch()
    {
        compute_line_timing();

        int fb_row = vce_line - fb_top_line;
        uint32_t *pfb = (fb_row >= 0 && fb_row < 240)
                            ? &fb[fb_row * max_width + frame_x]
                            : nullptr;

        if (vphase == VPHASE_VDW) {
            render_display_line(pfb, VDW + 1 - vphase_count);
        }
        else if (pfb) {
            std::fill_n(pfb, frame_width, rgb[pal[256]]);
        }

        //the hds irq point follows 8 dots later
        return 8 * dot_clock;
    }

    // vertical blank is raised here rather than at the vertical transition that
    // armed it, which leaves most of a line between it and the raster compare.
    // Cadash needs that gap - with both on the same instant its handler never
    // restores the background scroll and the throne room jumps.  the satb transfer
    // starts with the interrupt.
    int phase_hds_irq()
    {
        if (vblank_pending) {
            vblank_pending = false;
            vblank = 1;
            if (vdc_registers[0x05] & 0x8) {
                sys.irq1 = 1;
                vdc_status |= 0x20;
            }
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
        //on to the compare, 14 dots before the window closes
        return ph_hdw + 12 * dot_clock;
    }

    // the raster compare sits 14 dots before the display window closes, and the
    // vertical counter clocks with it - so vblank and the satb transfer are timed
    // from here too, not from the sync edge.
    int phase_rcr()
    {
        // rcr_counter names this line: $40 is the first display line, which is
        // what "add 64 to RCR to get the scanline" means, and it keeps counting
        // through blanking so it spans $40..$146 over a frame.
        if (vphase == VPHASE_VDS && vphase_count == 1) {
            rcr_counter = 0x40 - 1;
        }
        else {
            rcr_counter++;
        }

        advance_vertical();

        if (rcr_counter + 1 == (int)(vdc_registers[0x06] & 0x3FF)) {
            raster_compare = 1;
            if (vdc_registers[0x05] & 0x4) {
                sys.irq1 = 1;
                vdc_status |= 0x4;
            }
        }

        return line_master - (ph_hdw + 20 * dot_clock);
    }

    void advance_vertical()
    {
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
            vphase = VPHASE_VSW;
            VSW = vdc_registers[0x0C] & 0x1F;
            vphase_count = VSW + 1;
            burst_mode = !(vdc_registers[0x5] & 0xC0);
            frame_complete = true;
        }
    }

    bool take_frame_complete()
    {
        bool f = frame_complete;
        frame_complete = false;
        return f;
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
                read_buffer = *(uint16_t *)&vram[(MARR & 0x7FFF) * 2];
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
                byr_written = true;
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
                //HDW is bits 0-6 of HDR
                display_width = (vdc_registers[0x0B] & 0x7F) + 1;
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
                byr_written = true;
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
                //writing the high byte of LENR starts the transfer
                do_vram_dma();
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
    uint32_t fb[max_width * 240];
    // vce raster: defines the frame.  262 or 263 lines, from vce control bit 2.
    int vce_lines;
  private:
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
    bool frame_complete;
    // length of each horizontal phase for the current line, in master clocks
    int dot_clock = 4;
    static constexpr int line_master = 1365;
    int ph_hds;
    int ph_hdw;
    int ph_hde;
    int ph_hsw;

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
    int frame_width;
    int frame_x;
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
    bool byr_written;
    bool vblank_pending = false;

};
} //namespace tg16