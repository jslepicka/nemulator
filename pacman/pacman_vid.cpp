#include "pacman_vid.h"
#include "pacman.h"

const uint8_t c_pacman_vid::rg_weights[] = {0x0, 0x21, 0x47, 0x68, 0x97, 0xB8, 0xDE, 0xFF};
const uint8_t c_pacman_vid::b_weights[] = {0x0, 0x51, 0xAE, 0xFF};

c_pacman_vid::c_pacman_vid(c_pacman *pacman, int *irq)
{
    this->pacman = pacman;
    this->irq = irq;
    fb = std::make_unique<uint32_t[]>(288 * 224);
    vram = std::make_unique<uint8_t[]>(2048);
    sprite_ram = std::make_unique<uint8_t[]>(16);
    
    tile_rom = std::make_unique<uint8_t[]>(4096);
    sprite_rom = std::make_unique<uint8_t[]>(4096);
    color_rom = std::make_unique<uint8_t[]>(32);
    pal_rom = std::make_unique<uint8_t[]>(256);
    sprite_locs = std::make_unique<uint8_t[]>(16);
}

c_pacman_vid::~c_pacman_vid()
{
}


void c_pacman_vid::reset()
{
    line = 248;
    vid_address = 0;
    state = 0;
}

uint8_t c_pacman_vid::read_byte(uint16_t address)
{
    if (address <= 0x47FF) {
        return vram[address - 0x4000];
    }
    else if (address <= 0x4BFF) {
        return 0;
    }
    else if (address <= 0x4FFF) {
        return sprite_ram[address - 0x4FF0];
    }
    return 0;
}

void c_pacman_vid::write_byte(uint16_t address, uint8_t data)
{
    if (address <= 0x47FF) {
        vram[address - 0x4000] = data;
    }
    else if (address <= 0x4BFF) {
         //unused
    }
    else if (address <= 0x4FFF) {
        //sprite
        sprite_ram[address - 0x4FF0] = data;
    }
}

void c_pacman_vid::draw_background_line(int line)
{
    uint32_t *f = fb.get() + line * 288;

    vid_address = 0x3C2;
    vid_address += 0x1 * (line / 8);

    for (int i = 0; i < 2; i++) {
        draw_tile(f);
        vid_address += 0x20;
    }

    vid_address = 0x40;
    vid_address += 0x20 * (line / 8);

    for (int i = 0; i < 32; i++) {
        draw_tile(f);
        vid_address++;
    }

    vid_address = 0x02;
    vid_address += 0x1 * (line / 8);

    for (int i = 0; i < 2; i++) {
        draw_tile(f);
        vid_address += 0x20;
    }
}

void c_pacman_vid::draw_tile(uint32_t *&f)
{
    uint8_t tile_number = vram[vid_address];
    uint8_t pal_number = vram[vid_address + 0x400] & 0x3F;
    uint32_t chr_loc = tile_number * 16;
    chr_loc += (line % 8) + 8;
    uint8_t chr_data = tile_rom[chr_loc];
    for (int i = 0; i < 2; i++) {
        for (int x = 0; x < 4; x++) {
            uint8_t pixel = ((chr_data & 0x8) >> 3) | ((chr_data & 0x80) >> 6);
            chr_data <<= 1;
            //lookup color
            uint8_t color = lookup_color(pal_number, pixel);
            uint32_t rgb = colors[color];
            *f++ = rgb;
        }
        chr_data = tile_rom[chr_loc - 8];
    }
}

void c_pacman_vid::draw_sprite_line(int line)
{
    for (int i = 0; i < 8; i++) {
        int sprite_index = (i ^ 7);
        int sprite_offset = (i ^ 7) * 2;

        //x and y are from drawing perspective, not display perspective
        int sprite_line = sprite_locs[sprite_offset] - 31;

        //offset first _3_ sprites per mame source code (note: mame comment says 2, but loop is for 3)
        if (sprite_index < 3) {
            sprite_line += 1;
        }
        int sprite_x = 240 - sprite_locs[sprite_offset + 1] + 32;
        uint32_t j = line - sprite_line;
        if (j < 16) {
            //sprite is in range
            //get sprite number
            uint8_t sprite_register = sprite_ram[sprite_offset];
            uint8_t sprite_number = sprite_register >> 2;
            uint8_t sprite_y_flip = sprite_register & 0x1;
            uint8_t sprite_x_flip = (sprite_register >> 1) & 0x1;
            uint8_t sprite_pal = sprite_ram[sprite_offset + 1];

            uint32_t chr_loc = sprite_number * 64;
            uint32_t line_offset = line - sprite_line;
            if (sprite_x_flip) {
                line_offset ^= 7;
            }
            chr_loc += (line_offset % 8);

            //if j >= 8
            if (((j & 0x8) >> 3) ^ sprite_x_flip) {
                chr_loc += 32;
            }

            uint8_t chr_offset = sprite_y_flip ? 0 : 8;
            uint8_t chr_data;
            uint32_t *f = fb.get() + line * 288 + sprite_x;
            for (int x = 0; x < 16; x++) {
                if ((x & 0x3) == 0) {
                    int a = chr_loc + chr_offset;
                    chr_data = sprite_rom[a];

                    int chr_offset_adjust = sprite_y_flip ? -8 : 8;
                    chr_offset = (chr_offset + chr_offset_adjust) & 0x1F;
                }
                uint8_t pixel = 0;
                if (sprite_y_flip) {
                    pixel = ((chr_data & 0x1) >> 0) | ((chr_data & 0x10) >> 3);
                    chr_data >>= 1;
                }
                else {
                    pixel = ((chr_data & 0x8) >> 3) | ((chr_data & 0x80) >> 6);
                    chr_data <<= 1;
                }
                uint8_t color = lookup_color(sprite_pal, pixel);
                if (sprite_x + x < 288) {
                    if (color != 0) {
                        uint32_t rgb = colors[color];
                        *f = rgb;
                    }
                    f++;
                }
            }
        }
    }
}

uint8_t c_pacman_vid::lookup_color(int pal_number, int index)
{
    return pal_rom[pal_number * 4 + index] & 0x1F;
}

void c_pacman_vid::build_color_lookup()
{
    for (int i = 0; i < 32; i++) {
        colors[i] = lookup_rgb(i);
    }
}

uint32_t c_pacman_vid::lookup_rgb(uint8_t color)
{
   
    uint32_t rgb = color_rom[color];
    uint8_t r = rgb & 0x7;
    uint8_t g = (rgb >> 3) & 0x7;
    uint8_t b = (rgb >> 6) & 0x3;

    uint32_t out = rg_weights[r] | (rg_weights[g] << 8) | (b_weights[b] << 16);
    return out;
}

void c_pacman_vid::execute(int cycles)
{
    enum
    {
        START_VBLANK = 496,
        END_VBLANK = 272
    };

    if (line == START_VBLANK) {
        pacman->set_irq(1);
    }
    else if (line == END_VBLANK) {
        pacman->set_irq(0);
    }

    //ignore cycles and just render a line
    if (line <= 255) {
        //sync
    }
    else if (line <= 271) {
        //blanked
    }
    else if (line <= 495) {
        int out_line = line - 272;
        draw_background_line(out_line);
        draw_sprite_line(out_line);
    }
    else if (line <= 511) {
         //blanked
    }
    if (++line == 512) {
        line = 248;
    }
}