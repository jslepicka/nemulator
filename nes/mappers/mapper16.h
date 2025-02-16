#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper16 : public c_mapper, register_class<c_mapper_registry, c_mapper16>
{
public:
    c_mapper16(int submapper = 0);
    ~c_mapper16();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 16,
                .constructor = []() { return std::make_unique<c_mapper16>(); },
            },
            {
                .number = 159,
                .constructor = []() { return std::make_unique<c_mapper16>(1); },
            },
        };
    }
private:
    unsigned char *eprom;
    int irq_counter;
    int irq_enabled;
    int irq_asserted;
    int ticks;
};

} //namespace nes
