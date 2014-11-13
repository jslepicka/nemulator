#pragma once
class c_joypad
{
public:
	c_joypad(void) {};
	~c_joypad(void) {};
	int Reset(void);
	unsigned char ReadByte(unsigned short address);
	void WriteByte(unsigned short address, unsigned char value);
	unsigned char joy1, joy2, joy3, joy4;
private:
	int read1, read2;
	unsigned char strobe;
};