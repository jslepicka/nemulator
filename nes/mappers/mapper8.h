#pragma once
#include "..\mapper.h"

class c_mapper8 :
	public c_mapper
{
public:
	c_mapper8();
	~c_mapper8();
	void WriteByte(unsigned short address, unsigned char value);
	void reset();
};