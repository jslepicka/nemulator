#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper13 :
    public c_mapper
{
public:
    c_mapper13();
    ~c_mapper13();
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
};

} //namespace nes
