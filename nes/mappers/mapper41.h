#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper41 :
    public c_mapper
{
public:
    c_mapper41();
    ~c_mapper41() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    int enable_reg2;
    int chr;
};

} //namespace nes
