#pragma once
#include "..\mapper.h"

class c_mapper5 :
    public c_mapper
{
public:
    c_mapper5();
    ~c_mapper5();
    void WriteByte(unsigned short address, unsigned char value);
    unsigned char ReadByte(unsigned short address);
    void reset(void);
    void mmc5_ppu_write(unsigned short address, unsigned char value);
    void clock(int cycles);
private:
    int exramTileSelect;
    int exramColorSelect;
    int nt2000;
    int nt2400;
    int nt2800;
    int nt2c00;
    int splitTile;
    int splitSide;
    int splitEnable;
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
    unsigned char ReadChrRom(unsigned short address);
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

    int in_split_region();
};