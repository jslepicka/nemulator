#pragma once
#include "mapper4.h"

namespace nes {

class c_mapper118 :
    public c_mapper4
{
public:
    c_mapper118();
    ~c_mapper118() {};
    void WriteByte(unsigned short address, unsigned char value);
private:
    void Sync();
};

} //namespace nes
