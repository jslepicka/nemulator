#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper18 : public c_mapper, register_class<c_mapper_registry, c_mapper18>
{
public:
    c_mapper18();
    ~c_mapper18() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 18,
                .constructor = []() { return std::make_unique<c_mapper18>(); },
            }
        };
    }
private:
    int ticks;
    int irq_enabled;
    int irq_counter;
    int irq_reload;
    int irq_size;
    int irq_asserted;
    int irq_mask;
    void sync();
    int chr[8];
    int prg[3];
    void ack_irq();
};

} //namespace nes
