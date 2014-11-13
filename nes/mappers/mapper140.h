#pragma once
#include "..\mapper.h"

class c_mapper140 :
	public c_mapper
{
public:
	c_mapper140();
	~c_mapper140() {};
	void WriteByte(unsigned short address, unsigned char value);
};