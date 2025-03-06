module;
#include <stdint.h>
#include <atomic>
#include <memory>
export module sms:vdp;

namespace sms
{

class c_sms;

class c_vdp
{
  public:
    c_vdp(c_sms *sms);
    ~c_vdp(void);
    void write_data(unsigned char value);
    void write_control(unsigned char value);
    unsigned char read_data();
    unsigned char read_control();
    void reset();
    int *get_frame_buffer();
    void draw_scanline();
    int get_scanline();
    void eval_sprites();

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
    unsigned char line_counter;
    int line_irq;
    int frame_irq;
    unsigned char status;
    c_sms *sms;
    int control;
    int address;
    int address_latch_lo;
    int address_latch_hi;
    int address_flip_flop;
    int registers[16];
    int vram_write;
    std::unique_ptr<unsigned char[]> vram;
    std::unique_ptr<int[]> frame_buffer;
    //unsigned char *vram;
    unsigned char cram[64];
    unsigned char read_buffer;
    //int *frame_buffer;

    int lookup_color(int palette_index);

    void update_irq();
    static std::atomic<int> pal_built;
    void generate_palette();
    static uint32_t pal_sms[256];
    static uint32_t pal_gg[4096];
};

} //namespace sms