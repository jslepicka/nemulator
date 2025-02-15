#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper34 :
    public c_mapper
{
public:
    c_mapper34();
    ~c_mapper34();
    void WriteByte(unsigned short address, unsigned char value);
};

} //namespace nes
