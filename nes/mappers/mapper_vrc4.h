#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper_vrc4 : public c_mapper, register_class<nes_mapper_registry, c_mapper_vrc4>
{
public:
    c_mapper_vrc4(int submapper = 0);
    ~c_mapper_vrc4();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    void clock(int cycles);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 21,
                .name = "VRC4",
                .constructor = []() { return std::make_unique<c_mapper_vrc4>(1); },
            },
            {
                .number = 22,
                .name = "VRC4",
                .constructor = []() { return std::make_unique<c_mapper_vrc4>(3); },
            },
            {
                .number = 23,
                .name = "VRC4",
                .constructor = []() { return std::make_unique<c_mapper_vrc4>(); },
            },
            {
                .number = 25,
                .name = "VRC4",
                .constructor = []() { return std::make_unique<c_mapper_vrc4>(2); },
            },
        };
    }

  private:
    int swap_bits;
    int get_bits(int address);
    void sync();
    int chr[8];
    int irq_control;
    int irq_reload;
    int irq_counter;
    int irq_scaler;
    int reg_8;
    int reg_a;
    int prg_mode;
};

} //namespace nes
