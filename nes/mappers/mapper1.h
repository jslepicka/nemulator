#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper1 : public c_mapper, register_mapper<c_mapper1>
{
public:
    c_mapper1();
    ~c_mapper1();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 1,
                .name = "MMC1",
                .constructor = []() { return std::make_unique<c_mapper1>(); },
            }
        };
    }

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
