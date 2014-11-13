#pragma once
#include "..\mapper.h"

class c_mapper3 :
	public c_mapper
{
public:
	c_mapper3(void);
	~c_mapper3(void);
	void WriteByte(unsigned short address, unsigned char value);
	void reset();
};