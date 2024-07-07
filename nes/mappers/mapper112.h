#pragma once
#include "..\mapper.h"

class c_mapper112 :
    public c_mapper
{
public:
    c_mapper112();
    ~c_mapper112() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    int command;
};