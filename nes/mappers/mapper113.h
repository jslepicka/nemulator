#pragma once
#include "..\mapper.h"

class c_mapper113 :
    public c_mapper
{
public:
    c_mapper113();
    ~c_mapper113() {};
    void WriteByte(unsigned short address, unsigned char value);
};