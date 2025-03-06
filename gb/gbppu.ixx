module;

export module gb:ppu;
import std.compat;

namespace gb
{
export class c_gb;

class c_gbppu
{
  public:
    c_gbppu(c_gb *gb);
    ~c_gbppu();
    void execute(int cycles);
    void reset();
    uint8_t read_byte(uint16_t address);
    void write_byte(uint16_t address, uint8_t data);
    uint32_t *get_fb()
    {
        return fb.get();
    }
    void on_stop();

  private:
    c_gb *gb;
    int line;
    int current_cycle;
    int SCX;
    int SCY;
    int STAT;
    int LCDC;
    int BGP;
    int OBP0;
    int OBP1;
    int mode;
    int LY;
    int LYC;
    int WY;
    int WX;
    int DMA;

    int fetch_x;
    int window_tile;
    int window_start_line;

    int cpu_divider;

    std::unique_ptr<uint8_t[]> vram;
    std::unique_ptr<uint8_t[]> vram1;
    std::unique_ptr<uint8_t[]> oam;
    std::unique_ptr<uint32_t[]> fb;
    std::unique_ptr<uint32_t[]> fb_back;

    void eval_sprites(int y);
    struct s_sprite
    {
        uint8_t y;
        uint8_t x;
        uint8_t tile;
        uint8_t flags;
    } sprite_buffer[10];

    uint32_t *f;

    struct s_bg_fifo_entry
    {
        uint8_t valid;
        uint8_t pattern;
        uint8_t cgb_pal;
        uint8_t priority;
    } bg_fifo[8];

    struct s_obj_fifo_entry
    {
        uint8_t valid;
        uint8_t pattern;
        uint8_t dmg_pal;
        uint8_t priority;
        uint8_t cgb_pal;
        uint8_t oam_index;
    } obj_fifo[8];

    int obj_fifo_index;

    int bg_fifo_index;

    int p0_addr;
    int p1_addr;
    int first_tile;
    int char_addr;
    int fetch_phase;
    int obj_fetch_phase;
    int ybase;

    int current_pixel;
    int start_vblank;
    int dma_count;
    int sprite_count;
    int lcd_paused;
    int bg_latched;

    int in_sprite_window;

    int sprite_y_offset;
    int sprite_addr;

    int sprites_here;

    int start_hblank;

    int pixels_out;

    int stat_irq;
    int prev_stat_irq;

    int done_drawing;

    void update_stat();
    void set_ly(int line);

    void generate_color_lookup();
    static std::atomic<int> color_lookup_built;

    int in_window;

    int fetching_sprites;
    int SCX_latch;

    static const unsigned int palettes[][4];

    static uint32_t color_lookup[32];

    uint32_t KEY1;
    uint32_t double_speed;

    uint32_t OPRI;

    int hdma_general_count;
    int hdma_hblank_count;
    int hdma_length;

    uint8_t BCPS;
    uint8_t OBPS;
    uint8_t cgb_vram_bank;
    uint8_t tile;
    uint8_t obj_p0;
    uint8_t obj_p1;
    uint8_t bg_p0;
    uint8_t bg_p1;
    int8_t current_sprite;
    uint8_t sprite_tile;
    uint8_t HDMA1;
    uint8_t HDMA2;
    uint8_t HDMA3;
    uint8_t HDMA4;
    uint8_t HDMA5;
    uint8_t cgb_bg_attr;
    uint16_t hdma_source;
    uint16_t hdma_dest;

    const unsigned int *palette;
    alignas(8) uint8_t cgb_bg_pal[64];
    alignas(8) uint8_t cgb_ob_pal[64];

    void do_bg_fetch();
    void do_obj_fetch();
    void output_pixel();
    void exec_mode0();
    void exec_mode1();
    void exec_mode2();
    void exec_mode3();
};

} //namespace gb