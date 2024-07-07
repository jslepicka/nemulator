#pragma once
#include "..\mapper.h"

class c_mapper15 :
    public c_mapper
{
public:
    c_mapper15();
    ~c_mapper15() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    void sync();
    int reg;
    int mode;
};