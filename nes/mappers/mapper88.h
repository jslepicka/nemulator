#pragma once
#include "..\mapper.h"

class c_mapper88 :
    public c_mapper
{
public:
    c_mapper88();
    ~c_mapper88() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    unsigned char command;
};