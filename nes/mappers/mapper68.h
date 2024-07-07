#pragma once
#include "..\mapper.h"

class c_mapper68 :
    public c_mapper
{
public:
    c_mapper68();
    ~c_mapper68();
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
    void ppu_write(unsigned short address, unsigned char value);
private:
    int reg_c;
    int reg_d;
    int mirroring_mode;
    int nt_mode;
};