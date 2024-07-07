#pragma once
#include "..\mapper.h"

class c_mapper78 :
    public c_mapper
{
public:
    c_mapper78();
    ~c_mapper78() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    int mirror_mode;
};