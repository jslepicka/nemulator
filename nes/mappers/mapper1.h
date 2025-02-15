#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper1 :
    public c_mapper
{
public:
    c_mapper1();
    ~c_mapper1();
    void WriteByte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
protected:
    int ignore_writes;
    int cycle_count;
    void Sync();
    virtual void sync_prg();
    virtual void sync_chr();
    void sync_mirror();
    unsigned char regs[4];

    int count;
    unsigned char val;
    struct c_mapperConfig
    {
        unsigned char mirroring : 1;
        unsigned char onescreen : 1;
        unsigned char prgarea : 1;
        unsigned char prgsize : 1;
        unsigned char vrom : 1;
        unsigned char unused : 3;
    } *config;
};

} //namespace nes
