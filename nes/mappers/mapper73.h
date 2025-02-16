#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper73 : public c_mapper, register_class<c_mapper_registry, c_mapper73>
{
public:
    c_mapper73();
    ~c_mapper73() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 73,
                .name = "VRC3",
                .constructor = []() { return std::make_unique<c_mapper73>(); },
            },
        };
    }

  private:
    int irq_counter;
    int irq_reload;
    int irq_mode;
    int irq_enabled;
    int irq_enable_on_ack;
    int irq_asserted;
    int ticks;
    void ack_irq();
};

} //namespace nes
