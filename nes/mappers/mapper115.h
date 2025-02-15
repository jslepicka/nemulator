#pragma once
#include "mapper4.h"

namespace nes {

class c_mapper115 :
    public c_mapper4
{
public:
    c_mapper115();
    ~c_mapper115() {};
    void WriteByte(unsigned short address, unsigned char value);
private:
    int reg1;
    void reset();
    void Sync();
};

} //namespace nes
