#pragma once
#include "..\mapper.h"

class c_mapper82 :
    public c_mapper
{
public:
    c_mapper82();
    ~c_mapper82() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    void sync();
    int chr_mode;
    int chr[6];
};