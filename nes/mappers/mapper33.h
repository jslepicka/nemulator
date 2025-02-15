#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper33 :
    public c_mapper
{
public:
    c_mapper33();
    ~c_mapper33() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
};

} //namespace nes
