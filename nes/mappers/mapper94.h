#pragma once
#include "..\mapper.h"

class c_mapper94 :
	public c_mapper
{
public:
	c_mapper94();
	~c_mapper94();
	void WriteByte(unsigned short address, unsigned char value);
	void reset();
};