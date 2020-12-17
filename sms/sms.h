#pragma once
#include "..\console.h"

class c_z80;
class c_vdp;
class c_psg;

class c_sms : public c_console
{
public:
	//c_sms(void);
	c_sms(int variant);
	~c_sms(void);
	int load_rom(char *filename);
	int emulate_frame();
	unsigned char read_byte(unsigned short address);
	unsigned short read_word(unsigned short address);
	void write_byte(unsigned short address, unsigned char value);
	void write_word(unsigned short address, unsigned short value);
	void write_port(int port, unsigned char value);
	unsigned char read_port(int port);
	int reset();
	int *get_video();
	int get_sound_bufs(const short **buf_l, const short **buf_r);
	int irq;
	int nmi;
	void set_audio_freq(double freq);
	int load();
	int is_loaded() { return loaded; }
	int get_crc() { return crc; }
	void enable_mixer();
	void disable_mixer();
	void set_input(int input);
	int get_overscan_color();
	int get_fb_width() { return 256; }
	int get_fb_height() { return 192; }
private:
	int type = 0;
	int psg_cycles;
	int has_sram = 0;
	unsigned int crc = 0;
	int joy = 0xFF;
	int loaded = 0;
	int ram_select;
	int nationalism;
	unsigned char *rom;
	c_z80 *z80;
	c_vdp *vdp;
	c_psg *psg;
	unsigned char *ram;
	int file_length;
	unsigned char *page[3];
	unsigned char cart_ram[16384];
	int load_sram();
	int save_sram();
	void get_sram_path(char *path);
	char sram_file_name[MAX_PATH];
	void catchup_psg(int end_frame);
};

