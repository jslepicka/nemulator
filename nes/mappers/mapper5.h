#pragma once
#include "..\mapper.h"
#include "..\apu.h"

namespace nes {

class c_mapper5 : public c_mapper, register_class<c_mapper_registry, c_mapper5>
{
public:
    c_mapper5();
    ~c_mapper5();
    void write_byte(unsigned short address, unsigned char value);
    unsigned char read_byte(unsigned short address);
    void reset();
    void clock(int cycles);
    float mix_audio(float sample);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 5,
                .name = "MMC5",
                .constructor = []() { return std::make_unique<c_mapper5>(); },
            }
        };
    }
private:
    class c_mmc5_square : public c_apu::c_square
    {
    public:
        int get_output();
    } squares[2];
    int frame_seq_counter;
    static const int CLOCKS_PER_FRAME_SEQ = 89489;
    void clock_frame();
    int ticks;
    //c_apu::c_square squares[2];
    enum {
        PCM_IRQ_MODE_WRITE,
        PCM_IRQ_MODE_READ
    };
    int pcm_irq_enabled;
    int pcm_irq_asserted;
    int pcm_irq_mode;
    int pcm_data;
    int fillTile;
    int fillColor;
    int prgMode;
    int chrMode;
    int irqPending;
    int irqCounter;
    int inFrame;
    int irqEnable;
    int irqTarget;
    void Sync();
    int banks[12];
    int lastBank;
    unsigned char *bankA[8];
    unsigned char *bankB[8];
    void SetChrBank1k(unsigned char *b[8], int bank, int value);
    void SetChrBank2k(unsigned char *b[8], int bank, int value);
    void SetChrBank4k(unsigned char *b[8], int bank, int value);
    void SetChrBank8k(unsigned char *b[8], int value);
    unsigned char read_chr(unsigned short address);
    unsigned char ppu_read(unsigned short address);
    void ppu_write(unsigned short address, unsigned char value);
    int last_tile;

    int prg_reg[4];

    unsigned char *prg_ram;
    unsigned char *exram;
    unsigned char *prg_6000;
    int chr_high;
    int prg_ram_protect[2];
    int exram_mode;
    int multiplicand;
    int multiplier;

    void SetPrgBank8k(int bank, int value);
    void SetPrgBank16k(int bank, int value);
    void SetPrgBank32k(int value);
    int irq_asserted;
    int using_fill_table;
    int drawing_enabled;
    int split_control;
    int split_scroll;
    int split_page;
    int cycle;
    int scroll_line;
    int scanline;
    int cycles_since_rendering;
    int read_buffer;
    int last_ppu_read;
    int fetch_count;
    int irq_q;
    int vscroll;
    int htile;
    int split_address;
    int in_split_region;
    int last_address;
    int last_address_match_count;
    int idle_count;
    int ppu_is_reading;
    int tile_fetch_count;
    //int in_split_region();
};

} //namespace nes
