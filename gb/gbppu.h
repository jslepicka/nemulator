#pragma once
#include <cstdint>
#include <queue>

class c_gb;
class c_gbppu
{
public:
	c_gbppu(c_gb *gb);
	~c_gbppu();
	void execute(int cycles);
	void reset();
	uint8_t read_byte(uint16_t address);
	void write_byte(uint16_t address, uint8_t data);
	uint32_t* get_fb() { return fb; }
private:
	c_gb* gb;
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
	int frame = 0;
	int fetch_x;
	int window_tile;
	int window_start_line;


	int cpu_divider;

	uint8_t* vram;
	uint8_t* oam;

	void eval_sprites(int y);
	struct s_sprite {
		uint8_t y;
		uint8_t x;
		uint8_t tile;
		uint8_t flags;
	};

	s_sprite sprite_buffer[10];

	uint32_t* fb;
	uint32_t* fb_back;

	uint32_t* f;

	int oam_index;

	std::queue<int> sprite_fifo;

	int obj_fifo[8];
	int obj_fifo_index;
	int obj_fifo_count;

	int bg_fifo[8];
	int bg_fifo_index;

	uint8_t tile;
	int p0_addr;
	int p1_addr;
	uint8_t obj_p0;
	uint8_t obj_p1;
	int first_tile;
	int char_addr;
	int fetch_phase;
	int obj_fetch_phase;
	int ybase;
	uint8_t render_buf[8];
	int render_buf_index;
	int current_pixel;
	int start_vblank;
	int dma_count;
	int sprite_count;
	int lcd_paused;
	//int fetch_sprite;
	int fetched_sprite;
	int bg_latched;

	int in_sprite_window;

	int sprite_y_offset;
	int sprite_addr;

	int sprites_here;

	int start_hblank;

	uint8_t bg_p0;
	uint8_t bg_p1;

	int current_sprite;

	//int fetching_sprites;
	uint8_t sprite_tile;
	int pixels_out = 0;

	int stat_irq;
	int prev_stat_irq;

	int done_drawing;

	void update_stat();
	void set_ly(int line);

	int in_window;

	int fetching_sprites;
	int SCX_latch;

	static const unsigned int pal[][4];
	const unsigned int* shades;


	int dots = 0;
};

