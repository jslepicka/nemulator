#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper80 :
    public c_mapper
{
public:
    c_mapper80();
    ~c_mapper80() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
};

} //namespace nes
