#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper87 :
    public c_mapper
{
public:
    c_mapper87();
    ~c_mapper87() {};
    void WriteByte(unsigned short address, unsigned char value);
};

} //namespace nes
