module;

export module genesis:vdp;
import nemulator.std;

namespace genesis
{
export class c_vdp
{
    typedef std::function<uint16_t(uint32_t)> read_word_t;
  public:
    c_vdp(uint8_t *ipl, read_word_t read_word_68k);
    ~c_vdp();
    void reset();
    uint16_t read_word(uint32_t address);
    uint8_t read_byte(uint32_t address);
    void write_word(uint32_t address, uint16_t value);
    void write_byte(uint32_t address, uint8_t value);
    void draw_scanline();
    int freeze_cpu;

  private:
    read_word_t read_word_68k;
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
    uint8_t vsram[80];
    uint8_t vram[64 * 1024];

    uint32_t lookup_color(uint32_t pal, uint32_t index);
    void do_68k_dma();

  public:
    uint32_t frame_buffer[320 * 224];
};
} //namespace genesis