#pragma once
#include "..\mapper.h"

class c_mapper73 :
	public c_mapper
{
public:
	c_mapper73();
	~c_mapper73() {};
	void WriteByte(unsigned short address, unsigned char value);
	void reset();
	void clock(int cycles);
private:
	int irq_counter;
	int irq_reload;
	int irq_mode;
	int irq_enabled;
	int irq_enable_on_ack;
	int irq_asserted;
	int ticks;
	void ack_irq();
};