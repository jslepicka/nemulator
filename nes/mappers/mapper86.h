#pragma once
#include "..\mapper.h"

class c_mapper86 :
	public c_mapper
{
public:
	c_mapper86();
	~c_mapper86() {};
	void WriteByte(unsigned short address, unsigned char value);
};