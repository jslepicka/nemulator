#pragma once
#include "..\mapper.h"

class c_mapper2 :
	public c_mapper
{
public:
	c_mapper2(void);
	~c_mapper2(void);
	void WriteByte(unsigned short address, unsigned char value);
	void reset();
};