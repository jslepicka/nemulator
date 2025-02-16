#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper40 : public c_mapper, register_class<c_mapper_registry, c_mapper40>
{
public:
    c_mapper40();
    ~c_mapper40();
    void write_byte(unsigned short address, unsigned char value);
    unsigned char read_byte(unsigned short address);
    void reset();
    void clock(int cycles);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 40,
                .constructor = []() { return std::make_unique<c_mapper40>(); },
            }
        };
    }
private:
    int irq_counter;
    int irq_enabled;
    int ticks;
};

} //namespace nes
