#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper193 :
    public c_mapper
{
public:
    c_mapper193();
    ~c_mapper193() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
};

} //namespace nes
