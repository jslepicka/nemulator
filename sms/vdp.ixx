module;
#include <cassert>
#define USE_BMI
#ifdef USE_BMI
#include <immintrin.h>
#endif
export module sms:vdp;
import nemulator.std;
import :common;


namespace sms
{

export class i_vdp
{
  public:
    virtual void write_data(unsigned char value) = 0;
    virtual void write_control(unsigned char value) = 0;
    virtual unsigned char read_data() = 0;
    virtual unsigned char read_control() = 0;
    virtual void reset() = 0;
    virtual int *get_frame_buffer() = 0;
    virtual void draw_scanline() = 0;
    virtual int get_scanline() = 0;
    virtual void eval_sprites() = 0;
    virtual void latch_hscroll() = 0;
    virtual void line_irqs() = 0;
    virtual void end_line() = 0;
    virtual ~i_vdp() = default;

  protected:
    static inline std::atomic<int> pal_built = 0;
    static inline uint32_t pal_sms[256];
    static inline uint32_t pal_gg[4096];
};

export template <SMS_MODEL model>
class c_vdp : public i_vdp
{
  public:
    c_vdp(int *irq)
    {
        vram = std::make_unique_for_overwrite<unsigned char[]>(16384);
        frame_buffer = std::make_unique_for_overwrite<int[]>(256 * 256);
        generate_palette();
        this->irq = irq;
    }

    void write_data(unsigned char value)
    {
        read_buffer = value;
        int cram_mask;
        if constexpr (model == SMS_MODEL::GAMEGEAR) {
            cram_mask = 0x3F;
        }
        else {
            cram_mask = 0x1F;
        }
        
        switch (control) {
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

    void write_control(unsigned char value)
    {
        if (address_flip_flop) {
            address_latch_hi = value & 0x3F;
            control = value >> 6;
            address = address_latch_lo | (address_latch_hi << 8);
            switch (control) {
                case 0x0:
                    read_buffer = vram[address];
                    address = (address + 1) & 0x3FFF;
                    break;
                case 0x1: {
                    int x = 1;
                } break;
                case 0x2: {
                    registers[value & 0xF] = address & 0xFF;
                    update_irq();
                } break;
                case 0x3:
                    break;
            }
        }
        else {
            address_latch_lo = value;
            address = address_latch_lo | (address_latch_hi << 8);
        }
        address_flip_flop ^= 1;
    }

    unsigned char read_data()
    {
        address_flip_flop = 0;
        unsigned char b = read_buffer;
        read_buffer = vram[address++ & 0x3FFF];
        return b;
    }

    unsigned char read_control()
    {
        int ret = status;
        address_flip_flop = 0;
        status &= (~0xE0);
        line_irq = 0;
        frame_irq = 0;
        update_irq();
        return ret;
    }

    void reset()
    {
        sprite_count = 0;
        line_number = 0;
        line_irq = 0;
        frame_irq = 0;
        line_counter = 255;
        hscroll_latch = 0;
        control = 0;
        address = 0;
        address_latch_lo = 0;
        address_latch_hi = 0;
        address_flip_flop = 0;
        read_buffer = 0;
        vram_write = 0;
        std::memset(registers, 0, sizeof(registers));
        std::memset(vram.get(), 0, 16384);
        std::memset(cram, 0, sizeof(cram));
        status = 0;
    }

    int *get_frame_buffer()
    {
        return frame_buffer.get();
    }

    void draw_scanline()
    {
        //TODO: test zoomed sprites.  Apparently no SMS games use them?
        //Bock's Birthday 2006
        int sprite_width = registers[0x1] & 0x1 ? 16 : 8;

        int y = line_number;
        int active_height = get_active_height();

        //The GG LCD shows the middle 144 lines.  In 224-line mode, shift the output up
        //so that the visible area lands in the same place in the frame buffer.
        int gg_top = 24;
        int fb_row = y;
        if constexpr (model == SMS_MODEL::GAMEGEAR) {
            if (active_height == 224) {
                gg_top = 40;
                fb_row = (y - 16) & 0xFF;
            }
        }
        int *fb_line = &frame_buffer[fb_row * 256];

        if (y < active_height) {
            if (registers[0x1] & 0x40) {
                int x_coarse = hscroll_latch >> 3;
                int x_fine = hscroll_latch & 0x7;
                if (y < 16 && registers[0] & 0x40) {
                    x_coarse = x_fine = 0;
                }
                int y_adjusted = y + registers[9];
                if (active_height == 224) {
                    y_adjusted &= 0xFF;
                }
                else {
                    y_adjusted %= 224;
                }
                int y_offset = y_adjusted % 8;
                int y_address = (y_adjusted / 8) * 64;
                int background_color = lookup_color((registers[7] & 0xF) | 0x10);
                for (int i = 0; i < 8; i++) {
                    fb_line[i] = background_color;
                }
                for (int column = 0; column < 32; column++) {
                    unsigned int nt_column = (column - x_coarse) & 0x1F;
                    int nt_address;
                    if (active_height == 224) {
                        //32 row name table at 0x700 offset
                        nt_address = (((registers[2] & 0xC) << 10) | 0x700) + y_address + (nt_column * 2);
                    }
                    else {
                        nt_address = (y_address + (nt_column * 2)) & 0x7FF;
                        nt_address |= ((registers[2] & 0xE) << 10);
                    }

                    unsigned char nt_low = vram[nt_address];
                    unsigned char nt_hi = vram[nt_address + 1];
                    int tile_data = (nt_hi << 8) | nt_low;
                    int palette_select = (tile_data & 0x800) >> 7;
                    int pattern_number = tile_data & 0x1FF;
                    int h_flip = tile_data & 0x0200 ? 7 : 0;
                    int v_flip = tile_data & 0x0400 ? 7 : 0;
                    int priority = tile_data & 0x1000;
                    unsigned char *pattern = &vram[(pattern_number * 32)];

                    pattern += ((y_offset ^ v_flip) * 4);
                    int x = ((column * 8) + x_fine) & 0xFF;

                    int pat = *((int *)pattern);

                    int *fb = &fb_line[x];
                    for (int p = 0; p < 8 && x < 256; p++, x++) {
                        int pat2 = pat << (p ^ h_flip);
#ifdef USE_BMI
                        int pal_index = _pext_u32(pat2, 0b10000000'10000000'10000000'10000000);
#else
                        pat2 &= 0b10000000'10000000'10000000'10000000;
                        int pal_index = (pat2 >> 7) | (pat2 >> 14) | (pat2 >> 21) | (pat2 >> 28);
                        pal_index &= 0xF;
#endif

                        //sprites
                        int color = 0;
                        for (int i = 0; i < sprite_count; i++) {
                            if (x >= sprite_data[i].x && x < sprite_data[i].x + sprite_width) {
                                int pixel = x - sprite_data[i].x;
                                if (sprite_width == 16)
                                    pixel >>= 1;
                                int c = sprite_data[i].pixels[pixel];
                                if ((color & 0xF) == 0) {
                                    color = c;
                                }
                                else {
                                    status |= 0x20;
                                }
                            }
                        }

                        auto get_color = [&]() {
                            if (x < 8 && (registers[0] & 0x20)) {
                                return background_color;
                            }
                            else if ((color & 0xF) == 0 || (priority && pal_index != 0)) {
                                return lookup_color(pal_index | palette_select);
                            }
                            else
                                return lookup_color(color | 0x10);
                        };

                        if constexpr (model == SMS_MODEL::GAMEGEAR) {
                            if (y < gg_top || y >= gg_top + 144 || x < 48 || x >= 208) {
                                color = 0;
                            }
                            else {
                                color = get_color();
                            }
                        }
                        else {
                            color = get_color();
                        }

                        *fb++ = color;
                    }
                }
            }
            else {
                for (int x = 0; x < 256; x++) {
                    fb_line[x] = 0;
                }
            }
        }
        else if (y < 256) {
            for (int x = 0; x < 256; x++) {
                fb_line[x] = 0;
            }
        }
    }

    //Horizontal scroll is latched for the next line at vdp cycle 313.
    void latch_hscroll()
    {
        hscroll_latch = registers[8];
    }

    //The frame interrupt (vdp cycle 315, v counter $C1 in 192-line mode) and the line
    //counter (vdp cycle 316, for the lines leading into v counter 0 to active_height).
    void line_irqs()
    {
        int active_height = get_active_height();
        if (line_number == active_height) {
            status |= 0x80;
            update_irq();
        }

        if (line_number < active_height || line_number == 261) {
            line_counter--;
            if (line_counter == 0xFF) {
                line_counter = registers[10];
                line_irq = 1;
                update_irq();
            }
        }
        else {
            line_counter = registers[10];
        }
    }

    //The v counter moves to the next line at vdp cycle 315.
    void end_line()
    {
        if (line_number == 261)
            line_number = 0;
        else
            line_number++;
    }

    int get_scanline()
    {
        //NTSC v counter: 00-DA, D5-FF (192 lines) or 00-EA, E5-FF (224 lines)
        int jump = get_active_height() == 224 ? 0xEA : 0xDA;
        if (line_number <= jump)
            return line_number;
        else
            return (line_number - 6);
    }

    void eval_sprites()
    {
        sprite_count = 0;
        int sat_base = registers[0x5] & ~0x81;
        sat_base <<= 7;
        unsigned char *sat = &vram[sat_base];
        int sprite_height = registers[0x1] & 0x2 ? 16 : 8;
        if (registers[0x1] & 0x1) {
            sprite_height *= 2;
        }
        //sprite list terminator only exists in 192-line mode
        //https://www.smspower.org/Development/Sprites
        bool check_terminator = get_active_height() == 192;
        for (int i = 0; i < 64; i++) {
            unsigned char sprite_y = *(sat + i);

            if (sprite_y == 0xD0 && check_terminator)
                break;
            int sprite_y_adjusted = 0;

            //y values >= 0xF1 are treated as negative
            if (sprite_y >= 0xF1)
                sprite_y_adjusted = (char)sprite_y;
            else
                sprite_y_adjusted = sprite_y;

            sprite_y_adjusted += 1;
            int l = line_number;

            if (l >= (sprite_y_adjusted) && l < (sprite_y_adjusted) + sprite_height) {
                //sprite is in range
                int sprite_x = *(sat + 128 + i * 2);
                sprite_x -= registers[0] & 0x8;
                int sprite_pattern = *(sat + 129 + i * 2);
                if (registers[0x1] & 0x2) {
                    sprite_pattern &= ~0x1;
                }
                if (registers[0x6] & 0x4) {
                    sprite_pattern |= 0x100;
                }
                if (sprite_count < 8) {
                    sprite_data[sprite_count].x = sprite_x;
                    sprite_data[sprite_count].y = sprite_y;
                    sprite_data[sprite_count].pattern = sprite_pattern;
                    int sprite_base = (registers[0x6] & 0x4) << 10;
                    unsigned char *pattern = &vram[/*sprite_base |*/ (sprite_pattern * 32)];
                    int yy = l - sprite_y_adjusted;
                    if (registers[0x1] & 0x1) //if 8x16 sprite
                        yy /= 2;
                    pattern += (yy * 4);
                    for (int p = 0; p < 8; p++) {
                        int pal_index = 0;
                        for (int ii = 0; ii < 4; ii++) {
                            pal_index |= ((*(pattern + ii) >> (7 - p)) & 0x1) << ii;
                        }
                        sprite_data[sprite_count].pixels[p] = pal_index;
                    }
                    sprite_count++;
                }
                else {
                    status |= 0x40;
                    break;
                }
            }
        }
    }

  private:
    int get_active_height() const
    {
        //224-line mode: M4, M2, and M1 set, M3 clear.  240-line mode (M3 instead of M1)
        //only works on PAL consoles, so it isn't supported.
        if ((registers[0] & 0x6) == 0x6 && (registers[1] & 0x18) == 0x10) {
            return 224;
        }
        return 192;
    }

    int lookup_color(int palette_index)
    {
        if constexpr (model == SMS_MODEL::SMS) {
            return pal_sms[cram[palette_index]];
        }
        else if constexpr (model == SMS_MODEL::GAMEGEAR) {
            palette_index <<= 1;
            return pal_gg[(cram[palette_index] & 0xFF) | ((cram[palette_index + 1] & 0xF) << 8)];
        }
        else {
            assert(0);
        }
    }

    void update_irq()
    {
        if (((registers[1] & 0x20) && (status & 0x80)) || ((registers[0] & 0x10) && (line_irq))) {
            *irq = 1;
        }
        else
            *irq = 0;
    }

    void generate_palette()
    {
        int expected = 0;
        if (pal_built.compare_exchange_strong(expected, 1)) {
            double gamma = 1.0;
            for (int i = 0; i < 256; i++) {
                int num_bits = 3;
                int r_bits = i & num_bits;
                int g_bits = (i >> 2) & num_bits;
                int b_bits = (i >> 4) & num_bits;

                double r = pow(std::clamp(r_bits * (1.0 / (double)num_bits), 0.0, 1.0), gamma);
                double g = pow(std::clamp(g_bits * (1.0 / (double)num_bits), 0.0, 1.0), gamma);
                double b = pow(std::clamp(b_bits * (1.0 / (double)num_bits), 0.0, 1.0), gamma);

                pal_sms[i] = (int)(255.0 * r) | ((int)(255.0 * g) << 8) | ((int)(255.0 * b) << 16) | 0xFF000000;
            }
            for (int i = 0; i < 4096; i++) {
                int num_bits = 15;
                int r_bits = i & num_bits;
                int g_bits = (i >> 4) & num_bits;
                int b_bits = (i >> 8) & num_bits;

                double r = pow(std::clamp(r_bits * (1.0 / (double)num_bits), 0.0, 1.0), gamma);
                double g = pow(std::clamp(g_bits * (1.0 / (double)num_bits), 0.0, 1.0), gamma);
                double b = pow(std::clamp(b_bits * (1.0 / (double)num_bits), 0.0, 1.0), gamma);

                pal_gg[i] = (int)(255.0 * r) | ((int)(255.0 * g) << 8) | ((int)(255.0 * b) << 16) | 0xFF000000;
            }
        }
    }

  private:
    int sprite_count; //number of sprites on line
    struct
    {
        int x;
        int y;
        int pattern;
        int pixels[8];
    } sprite_data[8];
    int line_number;
    int hscroll_latch;
    unsigned char line_counter;
    int line_irq;
    int frame_irq;
    unsigned char status;
    int control;
    int address;
    int address_latch_lo;
    int address_latch_hi;
    int address_flip_flop;
    int registers[16];
    int vram_write;
    std::unique_ptr<unsigned char[]> vram;
    std::unique_ptr<int[]> frame_buffer;
    unsigned char cram[64];
    unsigned char read_buffer;
    int *irq;

};

} //namespace sms