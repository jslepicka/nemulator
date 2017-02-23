#pragma once
#include "..\mapper.h"

class c_mapper190 :
	public c_mapper
{
public:
	c_mapper190(void);
	~c_mapper190(void);
	void WriteByte(unsigned short address, unsigned char value);
	unsigned char ReadByte(unsigned short address);
	void reset();
private:
	unsigned char *ram;
};