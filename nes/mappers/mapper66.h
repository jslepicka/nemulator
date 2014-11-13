#pragma once
#include "..\mapper.h"

class c_mapper66 :
	public c_mapper
{
public:
	c_mapper66(void);
	~c_mapper66(void);
	void WriteByte(unsigned short address, unsigned char value);
	void reset(void);
};