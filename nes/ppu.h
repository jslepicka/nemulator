#pragma once
#include "nes.h"
#include <atomic>
#include <memory>

#define NES_PPU_USE_SIMD

class c_ppu
{
public:
    c_ppu();
    ~c_ppu();
    unsigned char read_byte(int address);
    void write_byte(int address, unsigned char value);
    void reset();
    int get_sprite_size();
    int eval_sprites();
    void run_ppu_line();
    int *get_frame_buffer() { return frame_buffer.get(); }

    c_mapper *mapper;
    c_cpu *cpu;
    c_apu *apu;
    c_apu2 *apu2;

    //unsigned char* pSpriteMemory;
    std::unique_ptr<unsigned char[]> pSpriteMemory;
    int drawingBg;
    bool limit_sprites;

private:
    void inc_horizontal_address();
    void inc_vertical_address();
    void generate_palette();
    void update_vram_address();
    void build_lookup_tables();
    uint64_t interleave_bits_64(unsigned char odd, unsigned char even);
    bool drawing_enabled();
    void output_pixel();
    void output_blank_pixel();
    void begin_vblank();
    void end_vblank();

    void fetch();
    void do_cycle_events();

    int vram_update_delay;
    //ppu updates apparently happen on ppu cycle following completion of a cpu write
    //when wr/ce goes high.  This is effectively 3 cycles after wr/cr goes low on the
    //last cycle of the cpu opcode (i.e., the write cycle)
    static const int VRAM_UPDATE_DELAY = 1;// 3;
    int suppress_nmi;
    static uint64_t morton_odd_64[];
    static uint64_t morton_even_64[];
    static const int screen_offset = 2;
    int* p_frame;
    unsigned int current_cycle;
    int current_scanline;
    int sprites_visible;
    int warmed_up;
    int spriteMemAddress;
    static std::atomic<int> lookup_tables_built;
    int fetch_state;
    unsigned char readValue;
    bool hi;
    bool nmi_pending;
    int vramAddress, vramAddressLatch, fineX;
    int addressIncrement;
    unsigned char* control1, * control2, * status;
    int update_rendering;
    int next_rendering;
    int hit;

    int odd_frame;
    int intensity;
    int sprite_count;
    int sprite0_index;
    int end_cycle;
    int rendering;
    int on_screen;
    int tile;
    int pattern_address;
    int attribute_address;
    int attribute_shift;
    #ifdef NES_PPU_USE_SIMD
    uint64_t pattern1;
    uint64_t pattern2;
    #else
    uint32_t pattern1;
    uint32_t pattern2;
    #endif
    uint64_t attribute;
    int executed_cycles;
    uint32_t pixel_pipeline;
    int palette_mask; //for monochrome display
    int vid_out;
    int reload_v;

    enum FETCH_STATE {
        FETCH_BG = 0 << 3,
        FETCH_SPRITE = 1 << 3,
        FETCH_NT = 2 << 3,
        FETCH_IDLE = 3 << 3
    };
    struct
    {
        unsigned char nameTable : 2;
        bool verticalWrite : 1;
        bool spritePatternTableAddress : 1;
        bool screenPatternTableAddress : 1;
        bool spriteSize : 1;
        bool hitSwitch : 1;
        bool vBlankNmi : 1;
    } ppuControl1;
    struct
    {
        unsigned char unknown : 1;
        bool backgroundClipping : 1;
        bool spriteClipping : 1;
        bool backgroundSwitch : 1;
        bool spritesVisible : 1;
        unsigned char unknown2 : 3;
    } ppuControl2;
    struct
    {
        unsigned char unknown : 4;
        bool vramWrite : 1;
        bool spriteCount : 1;
        bool hitFlag : 1;
        bool vBlank : 1;
    } ppuStatus;



    static uint32_t pal[512];
    std::unique_ptr<unsigned char[]> image_palette;
    std::unique_ptr<unsigned char[]> sprite_buffer;
    std::unique_ptr<unsigned char[]> sprite_memory;
    std::unique_ptr<unsigned char[]> sprite_index_buffer;
    std::unique_ptr<int[]> frame_buffer;
    std::unique_ptr<unsigned char[]> index_buffer;
};