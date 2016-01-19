#pragma once
#include "..\mapper.h"

class c_mapper_vrc4 :
	public c_mapper
{
public:
	c_mapper_vrc4(int submapper = 0);
	~c_mapper_vrc4();
	void WriteByte(unsigned short address, unsigned char value);
	void reset(void);
	void clock(int cycles);
private:
	int swap_bits;
	int get_bits(int address);
	void sync();
	int chr[8];
	int irq_control;
	int irq_reload;
	int irq_counter;
	int irq_scaler;
	int reg_8;
	int reg_a;
	int prg_mode;
};