#pragma once
#include "..\mapper.h"

class c_mapper_nsf :
	public c_mapper
{
public:
	c_mapper_nsf(void);
	~c_mapper_nsf(void);
	void WriteByte(unsigned short address, unsigned char value);
	unsigned char ReadByte(unsigned short address);
	void reset(void);
	int LoadImage();
private:
	struct NSF_HEADER {
		char signature[5];
		unsigned char version;
		unsigned char song_count;
		unsigned char starting_song;
		unsigned short load_address;
		unsigned short init_address;
		unsigned short play_address;
		char name_song[32];
		char name_artist[32];
		char name_copyright[32];
		unsigned short ntsc_speed;
		unsigned char bank_init[8];
		unsigned short pal_speed;
		unsigned char region;
		unsigned char expansion;
		unsigned char nsf2_reserved;
		unsigned char length[3];
	} *header;
	unsigned char* banks[8];
	unsigned char* nsf_data;
	unsigned char* sram;
	static unsigned char PLAYER_ROM[512];
	unsigned char player_rom[512];
	unsigned char chr_ram[8192];

	void load_player();
	void WriteChrRom(unsigned short address, unsigned char value);
	unsigned char ReadChrRom(unsigned short address);
};