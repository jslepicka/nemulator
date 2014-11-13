#pragma once
#include "..\mapper.h"

class c_mapper146 :
	public c_mapper
{
public:
	c_mapper146();
	~c_mapper146() {};
	void WriteByte(unsigned short address, unsigned char value);
};