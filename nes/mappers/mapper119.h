#pragma once
#include "mapper4.h"

class c_mapper119 :
    public c_mapper4
{
public:
    c_mapper119();
    ~c_mapper119();
    void SetChrBank1k(int bank, int value);
    void WriteChrRom(unsigned short address, unsigned char value);
private:
    unsigned char *ram;
};