#pragma once
#include "..\mapper.h"

class c_mapper71 :
	public c_mapper
{
public:
	c_mapper71();
	~c_mapper71() {};
	void WriteByte(unsigned short address, unsigned char value);
	void reset();
	int enable_mirroring_control;
};