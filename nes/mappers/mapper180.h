#pragma once
#include "..\mapper.h"

class c_mapper180 :
    public c_mapper
{
public:
    c_mapper180();
    ~c_mapper180() {}
    void reset();
    void WriteByte(unsigned short address, unsigned char value);
};