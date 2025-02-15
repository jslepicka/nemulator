#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper103 :
    public c_mapper
{
public:
    c_mapper103();
    ~c_mapper103();
    void WriteByte(unsigned short address, unsigned char value);
    unsigned char ReadByte(unsigned short address);
    void reset();
private:
    int rom_mode;
    unsigned char *prg_6000;
    unsigned char *ram;
};

} //namespace nes
