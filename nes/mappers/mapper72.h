#pragma once
#include "..\mapper.h"

class c_mapper72 :
    public c_mapper
{
public:
    c_mapper72();
    ~c_mapper72() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    int latch;
};