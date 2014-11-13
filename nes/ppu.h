#pragma once
#include "nes.h"

class c_ppu
{
public:
	c_ppu(void);
	~c_ppu(void);
	void reset_hscroll();
	void reset_vscroll();
	unsigned char ReadByte(int address);
	void WriteByte(int address, unsigned char value);
	void Reset(void);
	int BeginVBlank(void);
	void EndVBlank(void);
	void DrawScanline(void);
	void ClearFrameBuffer(void);
	bool DrawingEnabled(void);
	void StartFrame(void);
	void StartScanline(void);
	void EndScanline(void);
	bool limitSprites;
	unsigned char *pSpriteMemory;
	bool DoA12(void);
	bool Run(int numCycles);
	c_mapper *mapper;
	c_cpu *cpu;
	c_apu *apu;
	c_apu2 *apu2;
	int *pFrameBuffer;
	int GetSpriteSize();
	int drawingBg;
	int sprite0_hit_location;
	void set_sprite0_hit() {ppuStatus.hitFlag = true; sprite0_hit_location = -1;};
	void run_ppu(int cycles);
	void run_ppu_new(int cycles);
	int eval_sprites();
	int executed_cycles;
	int current_cycle;
	int current_scanline;
	bool limit_sprites;
	//int get_mirroring_mode();
private:
	int rendering_state;
	int get_bus_address(int address);
	static const int FB_TRANSPARENTBG = 0x1000;
	static const int FB_CLIPPED = 0x200;
	static const int FB_DONOTDRAW = 0x400;
	static const int FB_SPRITEDRAWN = 0x800;

	int odd_frame;
	int palette_mask; //for monochrome display
	int intensity;
	int mirroring_mode;
	bool four_screen;
	unsigned char sprite_buffer[256/*32*/];
	//int sprite_index_buffer[512/*64*/];
	unsigned char sprite_index_buffer[512];
	int sprite_count;
	int sprite0_index;
	bool nmi_pending;

	int end_cycle;
	int rendering;
	int short_scanline;
	int on_screen;
	int tile;
	int pattern_address;
	int attribute_address;
	int attribute_shift;
	int attribute;
	unsigned char index_buffer[272];
	int tick;

	int pattern1;
	int pattern2;

	int InterleaveBits(unsigned char odd, unsigned char even);

	__int32 interleave_bits_32(unsigned char odd, unsigned char even);
	__int64 interleave_bits_64(unsigned char odd, unsigned char even);

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

	unsigned char *control1, *control2, *status;
	int frameBuffer[256*256];
	unsigned char imagePalette[32];
	int addressIncrement;
	bool hi;
	int spriteMemAddress;
	int linenumber;
	unsigned char readValue;
	unsigned char *nameTable[4];
	int vramAddress, vramAddressLatch, fineX;
	int bgPatternTableAddress;

	void DrawBackgroundLine(void);
	void DrawSpriteLine(void);
	int GetMirrorAddress(int address);
	void DrawSpriteLinePixel(const int pixel);
	void DrawBackgroundLinePixel(const int pixel);

	static int mortonOdd[];
	static int mortonEven[];

	static __int32 morton_odd_32[];
	static __int32 morton_even_32[];

	static __int64 morton_odd_64[];
	static __int64 morton_even_64[];

	static int attr_shift_table[0x400];
	static int attr_loc[0x400];

	void update_vram_address();

	void build_lookup_tables();

	static long lookup_tables_built;
	unsigned char image_palette[32];

	int *p_frame;
	int sprites_visible;
};