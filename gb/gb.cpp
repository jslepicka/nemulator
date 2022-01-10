#define NOMINMAX
//#include "windows.h"
#include "gb.h"
#include "lr35902.h"
#include "gbmapper.h"
#include "mbc1.h"
#include "mbc2.h"
#include "mbc3.h"
#include "gbppu.h"
#include "gbapu.h"
#include <algorithm>

void strip_extension(char* path);

const std::map<int, c_gb::s_mapper> c_gb::mapper_factory =
{
	{0, {[]() {return new c_gbmapper(); }, 0, 0}},
	{1, {[]() {return new c_mbc1(); }, 0, 0}},
	{2, {[]() {return new c_mbc1(); }, 1, 0}},
	{3, {[]() {return new c_mbc1(); }, 1, 1}},
	{5, {[]() {return new c_mbc2(); }, 0, 0}},
	{6, {[]() {return new c_mbc2(); }, 0, 1}}
};


c_gb::c_gb()
{
	cpu = new c_lr35902(this);
	ppu = new c_gbppu(this);
	apu = new c_gbapu(this);
	ram = new uint8_t[8192];
	memset(ram, 0, 8192);
	hram = new uint8_t[128];
	memset(hram, 0, 128);
	loaded = 0;
	mapper = 0;
	ram_size = 0;
}

c_gb::~c_gb()
{
	if (loaded && m->has_battery) {
		save_sram();
	}
	if (ram)
		delete[] ram;
	if (hram)
		delete[] hram;
	if (mapper)
		delete mapper;
	if (cpu)
		delete cpu;
	if (apu)
		delete apu;
	if (ppu)
		delete ppu;
	if (rom)
		delete rom;
}

int c_gb::reset()
{
	//if (m->has_battery)
	//{
	//	save_sram();
	//}
	cpu->reset(0x100);
	mapper->reset();
	ppu->reset();
	apu->reset();
	IE = 0;
	IF = 0;
	SB = 0;
	SC = 0;

	DIV = 0;
	TIMA = 0;
	TMA = 0;
	TAC = 0;
	divider = 0;
	last_TAC_out = 0;
	next_input = input = -1;

	JOY = 0xFF;
	serial_transfer_count = 0;
	last_serial_clock = 0;
	return 0;
}

int *c_gb::get_video()
{
	return (int*)ppu->get_fb();
}

void c_gb::set_audio_freq(double freq)
{
	apu->set_audio_rate(freq);
}

int c_gb::get_sound_bufs(const short** buf_l, const short** buf_r)
{
	return apu->get_buffers(buf_l, buf_r);
}

int c_gb::load()
{

	sprintf_s(pathFile, "%s\\%s", path, filename);
	char temp[MAX_PATH];
	sprintf_s(temp, "%s\\%s", sram_path, filename);
	strip_extension(temp);
	sprintf_s(sramPath, "%s.sav", temp);

	//uint8_t logo[] = {
	//0xCE,0xED,0x66,0x66,0xCC,0x0D,0x00,0x0B,
	//0x03,0x73,0x00,0x83,0x00,0x0C,0x00,0x0D,
	//0x00,0x08,0x11,0x1F,0x88,0x89,0x00,0x0E,
	//0xDC,0xCC,0x6E,0xE6,0xDD,0xDD,0xD9,0x99,
	//0xBB,0xBB,0x67,0x63,0x6E,0x0E,0xEC,0xCC,
	//0xDD,0xDC,0x99,0x9F,0xBB,0xB9,0x33,0x3E
	//};
	std::ifstream file;
	file.open(pathFile, std::fstream::in | std::fstream::binary);
	if (file.fail())
		return false;
	file.seekg(0, std::ios_base::end);
	int file_length = (int)file.tellg();

	//printf("read %d bytes\n", file_length);
	file.seekg(0, std::ios_base::beg);
	rom = new uint8_t[std::max(32768, file_length)];
	//memcpy(rom + 0x104, logo, sizeof(logo));

	file.read((char*)rom, file_length);
	file.close();

	memcpy(title, rom + 0x134, 16);
	cart_type = *(rom + 0x147);
	rom_size = *(rom + 0x148);
	header_ram_size = *(rom + 0x149);

	if (file_length != (32768 << rom_size)) {
		return 0;
	}


	//printf("title: %s\n", title);
	//printf("cart_type: %d\n", cart_type);
	//printf("rom_size: %d\n", 32768 << rom_size);
	//printf("ram_size: %d\n", ram_size);
	auto i = mapper_factory.find(cart_type);
	if (i == mapper_factory.end()) {
		return false;
	}
	m = (s_mapper*)&(i->second);
	mapper = m->mapper();

	mapper->rom = rom;

	if (m->has_ram && header_ram_size && header_ram_size < 6) {
		const int size[] = { 0, 2 * 1024, 8 * 1024, 32 * 1024, 128 * 1024, 64 * 1024 };
		//int s = size[ram_size];
		//cart_ram = new uint8_t[s];
		//memset(cart_ram, 0, s);
		//mapper->ram = cart_ram;
		ram_size = size[header_ram_size];
		mapper->config_ram(ram_size);
		if (m->has_battery) {
			load_sram();
		}
	}
	else if (cart_type == 5 || cart_type == 6) {
		//mbc2 has 512x4bits of RAM
		ram_size = 512;
		mapper->config_ram(ram_size);
		if (cart_type == 6)
			load_sram();
	}
	else {
		//disable battery save if anything is invalid
		m->has_battery = 0;
	}
	
	reset();
	loaded = 1;
	return file_length;
}

int c_gb::load_sram()
{
	std::ifstream file;
	file.open(sramPath, std::fstream::in | std::fstream::binary);
	if (file.fail()) {
		return 0;
	}
	file.seekg(0, std::ios_base::end);
	int file_length = (int)file.tellg();
	file.seekg(0, std::ios_base::beg);

	if (file_length != ram_size) {
		return 0;
	}

	file.read((char*)mapper->ram, ram_size);
	file.close();
	return 1;
}

int c_gb::save_sram()
{
	std::ofstream file;
	file.open(sramPath, std::fstream::trunc | std::fstream::binary);
	if (file.fail()) {
		return 0;
	}

	file.write((char*)mapper->ram, ram_size);
	file.close();
	return 1;
}

uint16_t c_gb::read_word(uint16_t address)
{
	uint8_t lo = read_byte(address);
	uint8_t hi = read_byte(address + 1);
	return lo | (hi << 8);
}

uint8_t c_gb::read_byte(uint16_t address)
{
	if (address < 0x8000) {
		return mapper->read_byte(address);
	}
	else if (address < 0xA000) {
		return ppu->read_byte(address);
	}
	else if (address < 0xC000) {
		return mapper->read_byte(address);
	}
	else if (address < 0xFDFF) {
		return ram[address & 0x1FFF];
	}
	else {
		if (address >= 0xFE00 && address < 0xFF80) {
			if (address == 0xFF00) {
				if ((JOY & 0x30) == 0x20) {
					return 0xE0 | (input & 0xF);// | (JOY & 0x30);
				}
				else if ((JOY & 0x30) == 0x10) {
					return 0xD0 | ((input >> 4) & 0xF);// | (JOY & 0x30);
				}
				else {
					return 0xFF;
				}
				//return 0xF;
			}
			if (address == 0xFF01) {
				return 0xFF;
			}
			if (address == 0xFF02) {
				return SC;
			}
			if (address == 0xFF04) {
				return (divider & 0xFF00) >> 8;
			}
			if (address == 0xFF05) {
				int x = 1;
				return TIMA;
			}
			if (address == 0xFF06) {
				int x = 1;
				return TMA;
			}
			if (address == 0xFF07) {
				int x = 1;
				return TAC;
			}
			//OAM, unusable, IO Ports
			if (address == 0xFF0F) {
				return IF;
			}
			if (address >= 0xFF10 && address < 0xFF40) {
				//sound
				return apu->read_byte(address);
			}
			if ((address & 0xFF40) == 0xFF40) {
				return ppu->read_byte(address);
			}
		}
		else if (address == 0xFFFF) {
			return IE;
		}
		else if (address >= 0xFF80) {
			return hram[address - 0xFF80];
		}
		//else {
		//	return ram[address & 0x1FFF];
		//}
	}
	return 0;
}

void c_gb::write_word(uint16_t address, uint16_t data)
{
	write_byte(address, data & 0xFF);
	write_byte(address + 1, data >> 8);
}

void c_gb::write_byte(uint16_t address, uint8_t data)
{
	if (address < 0x8000) {
		//printf("write to cart\n");
		mapper->write_byte(address, data);
		//ignore?
		return;
	}
	else if (address < 0xA000) {
		//printf("write to vram\n");
		ppu->write_byte(address, data);
		return;
	}
	else if (address < 0xC000) {
		//printf("write to cart ram\n");
		mapper->write_byte(address, data);
		return;
	}
	else if (address < 0xFDFF) {
		ram[address & 0x1FFF] = data;
		return;
	}
	else {
		if (address >= 0xFE00 && address < 0xFF80) {
			//OAM, unusable, IO Ports
			//printf("write to io area\n");
			if (address == 0xFF00) {
				JOY = (JOY & (~0x30)) | (data & 0x30);
				return;
			}
			if (address == 0xFF01) {
				SB = data;
				return;
			}
			if (address == 0xFF02) {
				SC = data;
				if ((SC & 0x81) == 0x81) {
					serial_transfer_count = 8;
				}
				return;
			}
			if (address == 0xFF04) {
				divider = 0;
				return;
			}
			if (address == 0xFF05) {
				TIMA = data;
				return;
			}
			if (address == 0xFF06) {
				int x = 1;
				TMA = data;
				return;
			}
			if (address == 0xFF07) {
				int x = 1;
				TAC = data;
				return;
			}
			if (address == 0xFF0F) {
				IF = data;
				return;
			}
			if (address >= 0xFF10 && address < 0xFF40) {
				//sound
				apu->write_byte(address, data);
				return;
			}
			if (address >= 0xFF40 && address < 0xFF50) {
				ppu->write_byte(address, data);
				return;
			}
			if (address >= 0xFE00 && address <= 0xFE9F) {
				//OAM
				ppu->write_byte(address, data);
				return;
			}
			if (address >= 0xFEA0 && address <= 0xFEFF) {
				//printf("unusuable memory area %04X ???\n", address);
				return;
			}
			//printf("write to unhandled address %04X\n", address);
			return;
		}
		else if (address == 0xFFFF) {
			IE = data;
			return;
		}
		else if (address >= 0xFF80) {
			hram[address - 0xFF80] = data;
			return;
		}
		else {
			ram[address & 0x1FFF] = data;
			return;
		}
	}
	//MessageBox(NULL, "blah", "blah", MB_OK);
	//exit(0);
}

void c_gb::clock_timer()
{
	//this is clocked @ 4.2MHz.  Should be ok to simply increment by 4
	//since all CPU cycles are multiples of 4, and side effects of incrementing
	//timer (register updates an interrupts) are only detectable on cpu cycle
	//boundaries.

	int TAC_out;
	int serial_clock;
	int divisors[] = { 0x200, 0x08, 0x20, 0x80 };
	divider += 4;
	TAC_out = divider & divisors[TAC & 0x3];

	//clock serial output @ 8kHz
	serial_clock = divider & 0x100;

	if (last_serial_clock && (!serial_clock)) {
		if (serial_transfer_count) {
			if (--serial_transfer_count == 0) {
				IF |= 0x8;
			}
		}
	}
	last_serial_clock = serial_clock;
	if (TAC_out && (TAC & 0x4)) {
		TAC_out = 1;
	}
	else {
		TAC_out = 0;
	}
	if (last_TAC_out && (!TAC_out)) {
		if (TIMA == 0xFF) {
			TIMA = TMA;
			IF |= 0x4;
		}
		else {
			TIMA = (TIMA + 1) & 0xFF;
		}
	}
	last_TAC_out = TAC_out;
}

int c_gb::emulate_frame()
{
	apu->clear_buffers();
	//70224 PPU cycles per scanline
	//divided by 4 to get CPU clock
	//for scanline renderer:
	//456 PPU cycles
	//114 CPU cycles
	//154 times
	//4.2MHz master clock
	//	divided by 2 to get ppu memory clock = 2.1MHz
	//	divided by 4 to get cpu clock = 1.05MHz
	for (int line = 0; line < 154; line++) {
		//update input on line 144, right before vblank
		//Wizards & Warriors reads input at line 16 and 144 when
		//paused.  If input is updated too early, the second read at 144
		//overwrites the change detected on line 16, and makes it
		//impossible to resume from pause.
		if (line == 0x90) {
			input = next_input;
			if ((input & 0xFF) != 0xFF) {
				IF |= 0x10;
			}
		}
		ppu->execute(456);
	}
	return 0;
}

uint32_t* c_gb::get_fb()
{
	return ppu->get_fb();
}

void c_gb::set_vblank_irq(int status) {
	if (status) {
		IF |= 1;
	}
	else {
		IF &= ~1;
	}
}

void c_gb::set_stat_irq(int status) {
	if (status) {
		IF |= 2;
	}
	else {
		IF &= ~2;
	}
}
void c_gb::set_input(int input)
{
	next_input = input;
}

void c_gb::enable_mixer()
{
	apu->enable_mixer();
}

void c_gb::disable_mixer()
{
	apu->disable_mixer();
}