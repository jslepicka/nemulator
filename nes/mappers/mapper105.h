#pragma once
#include "mapper1.h"

namespace nes {

class c_mapper105 : public c_mapper1, register_class<c_mapper_registry, c_mapper105>
{
public:
    c_mapper105();
    ~c_mapper105() {};
    void reset();
    void clock(int cycles);
    int get_nwc_time();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 105,
                .name = "NWC",
                .constructor = []() { return std::make_unique<c_mapper105>(); },
            },
        };
    }

  private:
    int init;
    int irq_counter;
    int irq_asserted;
    void sync_prg();
    void sync_chr();
    int ticks;
    static const int irq_trigger = 0x28000000;
};

} //namespace nes
