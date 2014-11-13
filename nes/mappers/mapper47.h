#pragma once
#include "mapper4.h"

class c_mapper47 :
	public c_mapper4
{
public:
	c_mapper47();
	~c_mapper47() {};
	void WriteByte(unsigned short address, unsigned char value);
	void reset();
};