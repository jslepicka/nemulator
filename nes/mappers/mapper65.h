#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper65 : public c_mapper, register_class<nes_mapper_registry, c_mapper65>
{
public:
    c_mapper65();
    ~c_mapper65() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 65,
                .constructor = []() { return std::make_unique<c_mapper65>(); },
            }
        };
    }
private:
    int irq_enabled;
    int irq_asserted;
    int irq_counter;
    int irq_reload_high;
    int irq_reload_low;
    int ticks;
};

} //namespace nes
