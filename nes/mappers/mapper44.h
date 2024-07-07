#pragma once
#include "mapper4.h"

class c_mapper44
    : public c_mapper4
{
public:
    c_mapper44();
    ~c_mapper44() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
};