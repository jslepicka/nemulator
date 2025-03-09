module;

export module nes:ppu;
import nemulator.std;


namespace nes
{
export class c_mapper;
export class c_cpu;
export class c_apu;

export class c_ppu
{
  public:
    c_ppu();
    ~c_ppu();
    unsigned char read_byte(int address);
    void write_byte(int address, unsigned char value);
    void reset();
    int get_sprite_size();
    void eval_sprites();
    void run_ppu_line();
    int *get_frame_buffer()
    {
        return (int *)frame_buffer;
    }
    bool get_sprite_limit()
    {
        return limit_sprites;
    }
    void set_sprite_limit(bool limit)
    {
        limit_sprites = limit;
    }
    uint8_t *get_sprite_memory();

    c_mapper *mapper;
    c_cpu *cpu;
    c_apu *apu;
    int drawing_bg;

  private:
    void inc_horizontal_address();
    void inc_vertical_address();
    void generate_palette();
    void update_vram_address();
    void build_lookup_tables();
    uint64_t interleave_bits_64(unsigned char odd, unsigned char even);
    bool drawing_enabled();
    int output_pixel_no_sprites();
    int output_pixel();
    int output_blank_pixel();
    struct s_bg_out
    {
        uint8_t bg_index;
        uint32_t pixel;
    };
    s_bg_out get_bg();
    void begin_vblank();
    void end_vblank();

    void fetch();
    int do_cycle_events();

    int vram_update_delay;
    //ppu updates apparently happen on ppu cycle following completion of a cpu write
    //when wr/ce goes high.  This is effectively 3 cycles after wr/cr goes low on the
    //last cycle of the cpu opcode (i.e., the write cycle)
    static const int VRAM_UPDATE_DELAY = 1;// 3;
    int suppress_nmi;
    static uint64_t morton_odd_64[];
    static uint64_t morton_even_64[];
    static const int screen_offset = 2;
    unsigned int current_cycle;
    int current_scanline;
    int warmed_up;
    int sprite_mem_address;
    static std::atomic<int> lookup_tables_built;
    int fetch_state;
    int on_screen;
    unsigned char read_value;
    bool hi;
    bool nmi_pending;
    int vram_address;
    int vram_address_latch;
    int fine_x;
    int address_increment;

    int update_rendering;
    int next_rendering;
    int hit;

    int odd_frame;
    int intensity;
    int sprite_count;
    int sprite0_index;
    int rendering;

    int tile;
    int pattern_address;
    int attribute_address;

    uint64_t pattern1;
    uint64_t pattern2;
    uint64_t attribute;
    int executed_cycles;
    int palette_mask; //for monochrome display
    int vid_out;
    int reload_v;
    int attribute_shift;
    enum FETCH_STATE
    {
        FETCH_BG = 0 << 3,
        FETCH_SPRITE = 1 << 3,
        FETCH_NT = 2 << 3,
        FETCH_IDLE = 3 << 3
    };
    bool limit_sprites;
    union {
        struct
        {
            unsigned char nt_base : 2;
            bool address_increment : 1;
            bool sprite_pt_address : 1;
            bool bg_pt_address : 1;
            bool sprite_size : 1;
            bool master_slave_select : 1;
            bool nmi_enable : 1;
        };
        unsigned char value;
    } PPUCTRL;
    union {
        struct
        {
            unsigned char greyscale : 1;
            bool left_bg_enable : 1;
            bool left_sprite_enable : 1;
            bool enable_bg : 1;
            bool enable_sprites : 1;
            bool red_emphasis : 1;
            bool green_emphasis : 1;
            bool blue_emphasis : 1;
        };
        struct
        {
            unsigned char : 5;
            unsigned char emphasis : 3;
        };
        unsigned char value;
    } PPUMASK;
    union {
        struct
        {
            unsigned char : 5;
            bool sprite_overflow : 1;
            bool sprite0_hit : 1;
            bool in_vblank : 1;
        };
        unsigned char value;
    } PPUSTATUS;
    struct s_sprite_data
    {
        uint8_t y;
        uint8_t tile;
        uint8_t attribute;
        uint8_t x;
    };

    alignas(64) static uint32_t pal[512];

    int (c_ppu::*p_output_pixel)();
    std::bitset<256> sprite_here;

    alignas(64) s_sprite_data sprite_buffer[64];
    alignas(64) uint8_t sprite_memory[256];
    alignas(64) uint8_t sprite_index_buffer[512];
    alignas(64) uint8_t index_buffer[272];
    uint8_t image_palette[32];
    alignas(64) uint32_t frame_buffer[256 * 240];
};

} //namespace nes