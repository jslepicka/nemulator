#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper232 :
    public c_mapper
{
public:
    c_mapper232();
    ~c_mapper232() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    int bank;
    int page;
    void sync();
};

} //namespace nes
