#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper190 :
    public c_mapper
{
public:
    c_mapper190();
    ~c_mapper190();
    void WriteByte(unsigned short address, unsigned char value);
    unsigned char ReadByte(unsigned short address);
    void reset();
private:
    unsigned char *ram;
};

} //namespace nes
