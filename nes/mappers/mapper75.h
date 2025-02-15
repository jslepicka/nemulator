#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper75 :
    public c_mapper
{
public:
    c_mapper75();
    ~c_mapper75() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    void sync();
    int chr0, chr1;
};

} //namespace nes
