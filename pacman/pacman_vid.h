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
    void build_color_lookup();

    uint32_t *fb;
    uint8_t *tile_rom;
    uint8_t *sprite_rom;
    uint8_t *color_rom;
    uint8_t *pal_rom;
    uint8_t *sprite_locs;
  
private:
    c_pacman *pacman;
    int line;
    int *irq;
    uint32_t vid_address;
    uint8_t *vram;
    uint8_t *sprite_ram;
    
    void draw_tile(uint32_t *&f);

    void draw_background_line(int line);
    void draw_sprite_line(int line);

    int pixels_out;

    uint8_t lookup_color(int pal_number, int index);
    uint32_t lookup_rgb(uint8_t color);

    static const uint8_t rg_weights[];
    static const uint8_t b_weights[];

    uint32_t colors[32];
};