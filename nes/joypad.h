#pragma once
class c_joypad
{
public:
    c_joypad() {};
    ~c_joypad() {};
    int reset();
    unsigned char read_byte(unsigned short address);
    void write_byte(unsigned short address, unsigned char value);
    unsigned char joy1, joy2, joy3, joy4;
private:
    int read1, read2;
    unsigned char strobe;
};