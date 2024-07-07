#pragma once
#include "mapper4.h"

class c_mapper189 :
    public c_mapper4
{
public:
    c_mapper189();
    ~c_mapper189() {};
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
private:
    virtual void Sync();
    int reg_a;
    int reg_b;
};