#pragma once
#include "..\mapper.h"

class c_mapper7 :
	public c_mapper
{
public:
	c_mapper7(void);
	~c_mapper7(void);
	void WriteByte(unsigned short address, unsigned char value);
	void reset(void);
};