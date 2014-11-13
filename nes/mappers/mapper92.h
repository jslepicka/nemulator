#pragma once
#include "..\mapper.h"

class c_mapper92 :
	public c_mapper
{
public:
	c_mapper92();
	~c_mapper92() {};
	void WriteByte(unsigned short address, unsigned char value);
	void reset();
private:
	int latch;
};