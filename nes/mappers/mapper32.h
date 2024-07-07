#pragma once
#include "..\mapper.h"

class c_mapper32 :
    public c_mapper
{
public:
    c_mapper32();
    ~c_mapper32() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    int prg_reg0;
    int prg_reg1;
    int prg_mode;
    void sync();
};