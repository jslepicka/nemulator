#pragma once
#include "mapper4.h"

namespace nes {

class c_mapper95 :
    public c_mapper4
{
public:
    c_mapper95();
    ~c_mapper95() {};
    void WriteByte(unsigned short address, unsigned char value);
protected:
    void Sync();
};

} //namespace nes
