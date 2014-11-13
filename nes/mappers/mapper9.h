#pragma once
#include "..\mapper.h"

class c_mapper9 :
	public c_mapper
{
public:
	c_mapper9(void);
	virtual ~c_mapper9(void);
	virtual void WriteByte(unsigned short address, unsigned char value);
	virtual void reset(void);
	//void Latch(int address);
	void mmc2_latch(int address);
	unsigned char ReadChrRom(unsigned short address);
private:
	void sync();
	int latch0;
	int latch1;
	int latch0FD;
	int latch0FE;
	int latch1FD;
	int latch1FE;
	int latch_buffer[2];
	int latch_buffer_head;
	int latch_buffer_tail;
};