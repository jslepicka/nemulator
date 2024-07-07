#pragma once
#include "mapper4.h"

class c_mapper_mmc6 :
    public c_mapper4
{
public:
    c_mapper_mmc6();
    ~c_mapper_mmc6();
    void WriteByte(unsigned short addres, unsigned char value);
    unsigned char ReadByte(unsigned short address);
    void reset();
    int LoadImage();
protected:
    int wram_enable;
    int wram_control;
};