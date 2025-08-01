module;

export module genesis:vdp;
import nemulator.std;

namespace genesis
{
export class c_vdp
{
    typedef std::function<uint16_t(uint32_t)> read_word_t;
    typedef std::function<void(int)> mode_switch_callback_t;
  public:
    c_vdp(uint8_t *ipl, read_word_t read_word_68k, mode_switch_callback_t mode_switch_callback, uint32_t *stalled);
    ~c_vdp();
    void reset();
    uint16_t read_word(uint32_t address);
    uint8_t read_byte(uint32_t address);
    void write_word(uint32_t address, uint16_t value);
    void write_byte(uint32_t address, uint8_t value);
    void draw_scanline();
    int freeze_cpu;
    void ack_irq();
    void clear_hblank();

  private:
    uint32_t x_res;
    uint32_t *stalled;
    read_word_t read_word_68k;
    mode_switch_callback_t mode_switch_callback;
    uint8_t *ipl;
    uint16_t control;
    uint16_t hv_counter;
    uint16_t data;
    int line;
    uint8_t reg[32];
    int address_write;
    uint32_t address_reg;
    uint16_t _address;
    uint32_t dma_copy;
    uint32_t vram_to_vram_copy;
    enum class ADDRESS_TYPE
    {
        VRAM_READ = 0b0000,
        VRAM_WRITE = 0b0001,
        CRAM_WRITE = 0b0011,
        VSRAM_READ = 0b0100,
        VSRAM_WRITE = 0b0101,
        CRAM_READ = 0b1000,
    } address_type;
    union {
        struct
        {
            uint8_t pal : 1;
            uint8_t dma : 1;
            uint8_t hblank : 1;
            uint8_t vblank : 1;
            uint8_t odd_frame : 1;
            uint8_t sprite_collision : 1;
            uint8_t sprite_overflow : 1;
            uint8_t vint : 1;
            uint8_t fifo_full : 1;
            uint8_t fifo_empty : 1;
            uint8_t : 6;
        };
        uint16_t value;
    } status;
    uint8_t cram[128];
    uint32_t cram_entry[128];
    uint8_t vsram[80];
    uint8_t bg_color;

    void update_x_res();

    uint32_t lookup_color(uint32_t pal, uint32_t index);
    void do_68k_dma();
    uint32_t plane_width;
    uint32_t plane_height;
    uint32_t pending_fill;
    uint32_t asserting_vblank;
    uint32_t asserting_hblank;
    uint8_t hint_counter;
    void update_ipl();

    void draw_plane(
        uint8_t *out,
        uint32_t nt,
        uint32_t v_scroll,
        uint32_t h_scroll,
        uint32_t low_priority_val
        );
    uint16_t get_hscroll_loc();
    uint32_t rgb[512];
    uint8_t *plane_ptrs[4];
    alignas(64) uint8_t vram[64 * 1024];
    alignas(64) uint8_t a_out[328] = {0};
    alignas(64) uint8_t b_out[328] = {0};
    alignas(64) uint8_t win_out[328] = {0};
    alignas(64) uint8_t sprite_out[328] = {0};
    uint8_t padding[48];
    uint8_t priorities[336] = {0};
    
    void eval_sprites();
    
    enum LAYER_PRIORITY
    {
        SPRITE_HIGH = 0,
        WINDOW_HIGH = 1,
        A_HIGH      = 2,
        B_HIGH      = 3,
        SPRITE_LOW  = 4,
        WINDOW_LOW  = 5,
        A_LOW       = 6,
        B_LOW       = 7
    };

  public:
    alignas(64) uint32_t frame_buffer[320 * 224];
};
} //namespace genesis