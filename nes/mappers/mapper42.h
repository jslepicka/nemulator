#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper42 : public c_mapper, register_mapper<c_mapper42>
{
public:
    c_mapper42();
    ~c_mapper42() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
    unsigned char read_byte(unsigned short address);
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 42,
                .constructor = []() { return std::make_unique<c_mapper42>(); },
            }
        };
    }
private:
    int irq_enabled;
    int irq_asserted;
    int irq_counter;
    int ticks;
    unsigned char *prg_6k;
};

} //namespace nes
