#pragma once
#include "nes.h"
#include <atomic>

class c_ppu
{
public:
	c_ppu(void);
	~c_ppu(void);
	unsigned char read_byte(int address);
	void write_byte(int address, unsigned char value);
	void reset(void);
	int get_sprite_size();
	int eval_sprites();
	void run_ppu_line();

	c_mapper *mapper;
	c_cpu *cpu;
	c_apu *apu;
	c_apu2 *apu2;

	int *pFrameBuffer;
	unsigned char* pSpriteMemory;
	int drawingBg;
	bool limit_sprites;

private:
	void inc_horizontal_address();
	void inc_vertical_address();
	void generate_palette();
	void update_vram_address();
	void build_lookup_tables();
	uint64_t interleave_bits_64(unsigned char odd, unsigned char even);
	bool drawing_enabled(void);

	int vram_update_delay;
	static const int VRAM_UPDATE_DELAY = 3;
	int suppress_nmi;
	static uint64_t morton_odd_64[];
	static uint64_t morton_even_64[];
	int* p_frame;
	int sprites_visible;
	int warmed_up;
	int spriteMemAddress;
	static std::atomic<int> lookup_tables_built;
	int fetch_state;
	unsigned char readValue;
	int vramAddress, vramAddressLatch, fineX;
	int addressIncrement;
	bool hi;
	unsigned char* control1, * control2, * status;
	uint32_t pattern1;
	uint32_t pattern2;


	int odd_frame;
	int palette_mask; //for monochrome display
	int intensity;
	int sprite_count;
	int sprite0_index;
	bool nmi_pending;
	int end_cycle;
	int rendering;
	int on_screen;
	int tile;
	int pattern_address;
	int attribute_address;
	int attribute_shift;
	unsigned int attribute;
	int executed_cycles;
	unsigned int current_cycle;
	int current_scanline;

	enum FETCH_STATE {
		FETCH_IDLE,
		FETCH_SPRITE,
		FETCH_BG,
		FETCH_NT
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

	unsigned char image_palette[32];
	unsigned char sprite_buffer[256];
	unsigned char sprite_index_buffer[512];
	static uint32_t pal[512];
	static int attr_shift_table[0x400];
	static int attr_loc[0x400];
	int frameBuffer[256 * 256];
	unsigned char index_buffer[272];
};