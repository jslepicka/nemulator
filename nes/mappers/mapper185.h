#pragma once
#include "..\mapper.h"

class c_mapper185 :
	public c_mapper
{
public:
	c_mapper185();
	~c_mapper185();
	void WriteByte(unsigned short address, unsigned char value);
	unsigned char ReadChrRom(unsigned short address);
	void reset();
private:
	int chr_protected;
};