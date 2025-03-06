module;
#include <cstdint>
#include <memory>

export module pacman:vid;

namespace pacman
{
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

    std::unique_ptr<uint32_t[]> fb;
    std::unique_ptr<uint8_t[]> tile_rom;
    std::unique_ptr<uint8_t[]> sprite_rom;
    std::unique_ptr<uint8_t[]> color_rom;
    std::unique_ptr<uint8_t[]> pal_rom;
    std::unique_ptr<uint8_t[]> sprite_locs;

  private:
    uint8_t lookup_color(int pal_number, int index);
    uint32_t lookup_rgb(uint8_t color);
    void draw_background_line(int line);
    void draw_sprite_line(int line);
    void draw_tile();

    c_pacman *pacman;
    int *irq;
    uint32_t *f;
    std::unique_ptr<uint8_t[]> vram;
    std::unique_ptr<uint8_t[]> sprite_ram;

    int line;
    uint32_t state = 0;
    uint32_t vid_address;
    int pixels_out;

    static const uint8_t rg_weights[];
    static const uint8_t b_weights[];

    uint32_t colors[32];
};
} //namespace pacman