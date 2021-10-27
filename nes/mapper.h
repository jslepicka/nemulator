#pragma once
#include "ines.h"
#include "windows.h"

class c_ppu;
class c_cpu;
class c_nes;
class c_apu2;

class c_mapper
{
public:
	c_mapper(void);
	virtual ~c_mapper(void);
	virtual unsigned char ReadByte(unsigned short address);
	virtual void WriteByte(unsigned short address, unsigned char value);
	virtual void WriteChrRom(unsigned short address, unsigned char value);
	virtual unsigned char ReadChrRom(unsigned short address);
	virtual void mmc3_check_a12() {};
	virtual void mmc5_ppu_write(unsigned short address, unsigned char value) {};
	virtual void mmc2_latch(int address) {};
	virtual void clock(int cycles) {};
	virtual void ppu_clock(int cycles) {};
	virtual void HBlank(int scanline) {};
	virtual void reset(void);
	virtual int LoadImage(void);
	virtual float mix_audio(float sample);
	int has_expansion_audio();
	c_ppu *ppu;
	c_cpu *cpu;
	c_apu2 *apu2;
	int renderingBg;
	void set_submapper(int submapper);
	iNesHeader* header;
	unsigned char *image;
	char filename[MAX_PATH];
	char sramFilename[MAX_PATH];
	const char *mapperName;
	int CloseSram();
	int crc32;
	virtual unsigned char ppu_read(unsigned short address);
	virtual void ppu_write(unsigned short address, unsigned char value);
	virtual void set_ppu_bus(unsigned short address) {};
	virtual int get_nwc_time() { return 0; }
	int in_sprite_eval;
	c_nes *nes;
	int get_mirroring();
protected:
	int expansion_audio;
	static const int MIRRORING_HORIZONTAL = 0;
	static const int MIRRORING_VERTICAL = 1;
	static const int MIRRORING_ONESCREEN_LOW = 2;
	static const int MIRRORING_ONESCREEN_HIGH = 3;
	static const int MIRRORING_FOURSCREEN = 4;

	static const int CHR_0000 = 0;
	static const int CHR_0400 = 1;
	static const int CHR_0800 = 2;
	static const int CHR_0C00 = 3;
	static const int CHR_1000 = 4;
	static const int CHR_1400 = 5;
	static const int CHR_1800 = 6;
	static const int CHR_1C00 = 7;

	static const int PRG_8000 = 0;
	static const int PRG_A000 = 1;
	static const int PRG_C000 = 2;
	static const int PRG_E000 = 3;

	bool hasSram;
	bool writeProtectSram;
	int sram_enabled;
	int four_screen;
	void set_mirroring(int mode);
	int mirroring_mode;

	unsigned char vram[4096];
	unsigned char *name_table[8];

	unsigned char *sram;
	int prgRomPageCount8k;
	int prgRomPageCount16k;
	int prgRomPageCount32k;
	int chrRomPageCount1k;
	int chrRomPageCount2k;
	int chrRomPageCount4k;

	typedef unsigned char prgRomBank[16384];
	typedef unsigned char chrRomBank[8192];
	prgRomBank *prgRom;
	chrRomBank *chrRom;
	unsigned char *pChrRom;
	unsigned char *pPrgRom;
	bool *IRQ;

	unsigned char *prgBank[4];
	unsigned char *chrBank[8];
	int *patternBank[8];

	void SetPrgBank8k(int bank, int value);
	void SetPrgBank16k(int bank, int value);
	void SetPrgBank32k(int value);
	virtual void SetChrBank1k(int bank, int value);
	void SetChrBank2k(int bank, int value);
	void SetChrBank4k(int bank, int value);
	void SetChrBank8k(int value);
	bool chrRam;
	bool dynamicImage;

	int OpenSram();

	int submapper;
};