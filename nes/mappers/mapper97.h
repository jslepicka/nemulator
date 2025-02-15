#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper97 :
    public c_mapper
{
public:
    c_mapper97();
    ~c_mapper97() {};
    void reset();
    void WriteByte(unsigned short address, unsigned char value);
};

} //namespace nes
