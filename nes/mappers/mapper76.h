#pragma once
#include "..\mapper.h"

class c_mapper76 :
	public c_mapper
{
public:
	c_mapper76();
	~c_mapper76() {};
	void WriteByte(unsigned short address, unsigned char value);
	void reset();
private:
	void sync();
	int prg_mode;
	int command;
};