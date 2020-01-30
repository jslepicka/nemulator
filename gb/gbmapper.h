#pragma once
#include <string>
#include <cstdint>
#include <fstream>
#include <cstdio>

class c_gbmapper
{
public:
	c_gbmapper();
	virtual ~c_gbmapper();
	//bool load_rom(std::string filename);
	virtual void write_byte(uint16_t address, uint8_t data);
	virtual uint8_t read_byte(uint16_t address);
	virtual void reset();
	virtual void config_ram(int ram_size);
	uint8_t* rom;
	uint8_t* ram;
private:
};

