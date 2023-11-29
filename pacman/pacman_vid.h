#pragma once
#include <cstdint>

class c_pacman;

class c_pacman_vid
{
  public:
    c_pacman_vid(c_pacman *pacman, int *irq);
    ~c_pacman_vid();
    void reset();
    uint8_t read_byte(uint16_t address);
    void write_byte(uint16_t address, uint8_t data);
    void execute(int cycles);
    uint32_t irq_enabled;
    int *fb;

    uint8_t *tile_rom;
    uint8_t *sprite_rom;
    uint8_t *color_rom;
    uint8_t *pal_rom;

    uint8_t *sprite_ram;

    uint8_t *sprite_locs;
    uint32_t irq_asserted;
  
private:
    c_pacman *pacman;
    int line;
    int *irq;
    uint32_t vid_address;
    uint8_t *vram;
    
    void draw_tile(int *&f);

    void draw_background_line(int line);
    void draw_sprite_line(int line);

    int pixels_out;

    uint8_t lookup_color(int pal_number, int index);
    uint32_t lookup_rgb(uint8_t color);

    static const uint8_t rg_weights[];
    static const uint8_t b_weights[];
};