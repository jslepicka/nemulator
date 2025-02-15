#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper152 :
    public c_mapper
{
public:
    c_mapper152();
    ~c_mapper152() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
};

} //namespace nes
