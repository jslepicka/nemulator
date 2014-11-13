#pragma once
#include "..\mapper.h"

class c_mapper8 :
	public c_mapper
{
public:
	c_mapper8(void);
	~c_mapper8(void);
	void WriteByte(unsigned short address, unsigned char value);
	void reset(void);
};