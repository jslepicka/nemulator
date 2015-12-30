#pragma once
#include "..\console.h"

class c_z80;
class c_vdp;
class c_psg;

class c_sms : public c_console
{
public:
	c_sms(void);
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
	int get_sound_buf(const short **buf);
	int irq;
	void set_audio_freq(double freq);
	int load();
	int is_loaded() { return loaded; }
	int get_crc() { return 0; }
	void enable_mixer();
	void disable_mixer();
private:
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
};

