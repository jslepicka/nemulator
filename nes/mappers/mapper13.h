#pragma once
#include "..\mapper.h"

class c_mapper13 :
	public c_mapper
{
public:
	c_mapper13(void);
	~c_mapper13(void);
	void WriteByte(unsigned short address, unsigned char value);
	void reset();
};