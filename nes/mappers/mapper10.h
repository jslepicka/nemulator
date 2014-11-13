#pragma once
#include "mapper9.h"

class c_mapper10:
	public c_mapper9
{
public:
	c_mapper10();
	~c_mapper10() {};
	void reset();
	void WriteByte(unsigned short address, unsigned char value);
};