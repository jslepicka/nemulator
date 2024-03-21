#pragma once
#include <cstdint>
#include <queue>
#include <atomic>
#include <memory>

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
	uint32_t* get_fb() { return fb.get(); }
    void on_stop();

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
    uint8_t BCPS;
    uint8_t OBPS;
    uint8_t cgb_vram_bank;
	int frame = 0;
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
	struct s_sprite {
		uint8_t y;
		uint8_t x;
		uint8_t tile;
		uint8_t flags;
	};

	s_sprite sprite_buffer[10];

	uint32_t* f;

	int oam_index;

	std::queue<int> sprite_fifo;

	uint32_t obj_fifo[8];
    uint8_t obj_fifo_pal[8];
	int obj_fifo_index;
	int obj_fifo_count;

	uint32_t bg_fifo[8];
    uint8_t bg_fifo_pal[8];
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

	void generate_color_lookup();
    static std::atomic<int> color_lookup_built;

	int in_window;

	int fetching_sprites;
	int SCX_latch;

	static const unsigned int pal[][4];
	const unsigned int* shades;


	int dots = 0;

	uint8_t cgb_bg_pal[64];
    uint8_t cgb_ob_pal[64];
    uint8_t cgb_bg_attr;

	static uint32_t color_lookup[32];

	uint32_t KEY1;
	uint32_t double_speed;

	uint32_t OPRI;
};

