#pragma once
#include <string>
#include <cstdint>
#include <map>
#include <functional>
#include "../console.h"

class c_lr35902;
class c_gbmapper;
class c_gbppu;
class c_gbapu;

class c_gb : public c_console
{
public:
	c_gb();
	~c_gb();
	//bool load_rom(std::string filename);
	int load();
	void write_byte(uint16_t address, uint8_t data);
	void write_word(uint16_t address, uint16_t data);
	uint8_t read_byte(uint16_t address);
	uint16_t read_word(uint16_t address);
	int reset();
	int emulate_frame();
	int IE; //interrput enable register
	int IF; //interrupt flag register
	c_lr35902* cpu;
	c_gbapu* apu;
	uint32_t* get_fb();
	void clock_timer();
	void set_vblank_irq(int status);
	void set_input(int input);
	void set_stat_irq(int status);

	int is_loaded() { return loaded; }
	int get_crc() { return 0; }
	int* get_video();

	int get_sound_bufs(const short** buf_l, const short** buf_r);
	void set_audio_freq(double freq);

	void enable_mixer();
	void disable_mixer();

private:

	c_gbppu* ppu;
	c_gbmapper* mapper;
	uint8_t* ram;
	uint8_t* hram;
	//uint8_t* cart_ram;
	int SB; //serial transfer data
	int SC; //serial transfer control

	uint8_t DIV;  //divider register
	uint8_t TIMA; //timer counter
	uint8_t TMA; //timer modulo
	uint8_t TAC; //timer control
	uint8_t JOY;
	int divider;
	int last_TAC_out;
	int input;
	int next_input;
	uint8_t* rom;

	uint8_t cart_type;
	uint8_t rom_size;
	uint8_t header_ram_size;
	int ram_size;
	char title[17] = { 0 };
	
	struct s_mapper {
		std::function<c_gbmapper * ()> mapper;
		int has_ram;
		int has_battery;
	};

	int load_sram();
	int save_sram();

	const static std::map <int, s_mapper> mapper_factory;
	s_mapper *m;
	int loaded;
	char sramPath[MAX_PATH];
};

