#pragma once
#include "..\mapper.h"

class c_mapper77 :
	public c_mapper
{
public:
	c_mapper77();
	~c_mapper77();
	void WriteByte(unsigned short address, unsigned char value);
	void reset();
private:
	unsigned char *chr_ram;
};