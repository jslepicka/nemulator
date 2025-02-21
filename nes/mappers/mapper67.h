#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper67 : public c_mapper, register_class<nes_mapper_registry, c_mapper67>
{
public:
    c_mapper67();
    ~c_mapper67();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 67,
                .constructor = []() { return std::make_unique<c_mapper67>(); },
            }
        };
    }
private:
    int irq_counter;
    int irq_enabled;
    int ticks;
    int irq_write;
};

} //namespace nes
