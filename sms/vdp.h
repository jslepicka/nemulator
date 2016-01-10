#pragma once
#include <stdint.h>
class c_sms;

class c_vdp
{
public:
	c_vdp(c_sms *sms);
	~c_vdp(void);
	void write_data(unsigned char value);
	void write_control(unsigned char value);
	unsigned char read_data();
	unsigned char read_control();
	void reset();
	int *get_frame_buffer();
	void draw_scanline();
	int get_scanline();
	void eval_sprites();
	int get_overscan_color();
private:
	int sprite_count; //number of sprites on line
	struct {
		int x;
		int y;
		int pattern;
		int pixels[8];
	} sprite_data[8];
	int line_number;
	unsigned char line_counter;
	int line_irq;
	int frame_irq;
	unsigned char status;
	c_sms *sms;
	int control;
	int address;
	int address_latch_lo;
	int address_latch_hi;
	int address_flip_flop;
	int registers[16];
	int vram_write;
	unsigned char *vram;
	unsigned char cram[32];
	unsigned char read_buffer;
	int *frame_buffer;

	int lookup_color(int palette_index);

	void update_irq();
	static long pal_built;
	void generate_palette();
	static uint32_t pal[256];
};

