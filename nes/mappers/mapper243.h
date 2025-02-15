#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper243 :
    public c_mapper
{
public:
    c_mapper243();
    ~c_mapper243() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    int chr;
    int command;
    void sync_chr();
};

} //namespace nes
