#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper19 : public c_mapper, register_class<c_mapper_registry, c_mapper19>
{
public:
    c_mapper19();
    ~c_mapper19();
    void write_byte(unsigned short address, unsigned char value);
    unsigned char read_byte(unsigned short address);
    void reset();
    void clock(int cycles);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 19,
                .constructor = []() { return std::make_unique<c_mapper19>(); },
            }
        };
    }
private:
    int ticks;
    void sync_chr();
    void sync_mirroring();
    int irq_counter;
    int irq_enabled;
    unsigned char *chr_ram;
    int chr_regs[8];
    int mirroring_regs[4];
    int reg_e800;
    int irq_asserted;
};

} //namespace nes
