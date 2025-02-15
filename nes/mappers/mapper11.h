#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper11 :
    public c_mapper
{
public:
    c_mapper11();
    ~c_mapper11();
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
};

} //namespace nes
