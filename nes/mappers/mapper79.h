#pragma once
#include "..\mapper.h"

class c_mapper79 :
	public c_mapper
{
public:
	c_mapper79();
	~c_mapper79();
	void WriteByte(unsigned short address, unsigned char value);
};