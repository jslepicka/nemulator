#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper66 :
    public c_mapper
{
public:
    c_mapper66();
    ~c_mapper66();
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
};

} //namespace nes
