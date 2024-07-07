#pragma once
#include "..\mapper.h"

class c_mapper228 :
    public c_mapper
{
public:
    c_mapper228();
    ~c_mapper228();
    void WriteByte(unsigned short address, unsigned char value);
    unsigned char ReadByte(unsigned short address);
    void reset();
private:
    int regs[4];
};