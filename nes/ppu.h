#pragma once
#include "nes.h"
#include <atomic>
#include <memory>
#include <immintrin.h>
#include <bitset>
#include <array>
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
    void eval_sprites();
    void run_ppu_line();
    int *get_frame_buffer() { return (int*)frame_buffer; }
    bool get_sprite_limit() { return limit_sprites; }
    void set_sprite_limit(bool limit) { limit_sprites = limit; }

    c_mapper *mapper;
    c_cpu *cpu;
    c_apu *apu;
    c_apu2 *apu2;

    uint8_t *get_sprite_memory();
    int drawingBg;

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
    int sprites_visible;
    int warmed_up;
    int spriteMemAddress;
    static std::atomic<int> lookup_tables_built;
    int fetch_state;
    int on_screen;
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
    
    int tile;
    int pattern_address;
    int attribute_address;
    
    #ifdef NES_PPU_USE_SIMD
    uint64_t pattern1;
    uint64_t pattern2;
    #else
    uint32_t pattern1;
    uint32_t pattern2;
    #endif
    uint64_t attribute;
    int executed_cycles;
    int palette_mask; //for monochrome display
    int vid_out;
    int reload_v;
    int attribute_shift;
    enum FETCH_STATE {
        FETCH_BG = 0 << 3,
        FETCH_SPRITE = 1 << 3,
        FETCH_NT = 2 << 3,
        FETCH_IDLE = 3 << 3
    };
    bool limit_sprites;
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