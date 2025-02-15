#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper89 :
    public c_mapper
{
public:
    c_mapper89();
    ~c_mapper89() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
};

} //namespace nes
