#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper2 :
    public c_mapper
{
public:
    c_mapper2();
    ~c_mapper2();
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
};

} //namespace nes
