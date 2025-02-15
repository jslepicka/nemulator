#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper184 :
    public c_mapper
{
public:
    c_mapper184();
    ~c_mapper184();
    void WriteByte(unsigned short address, unsigned char value);
};

} //namespace nes
