#pragma once
#include "..\mapper.h"

class c_mapper93 :
    public c_mapper
{
public:
    c_mapper93();
    ~c_mapper93();
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
};