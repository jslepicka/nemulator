#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper64 : public c_mapper, register_class<c_mapper_registry, c_mapper64>
{
public:
    c_mapper64();
    ~c_mapper64();
    void write_byte(unsigned short address, unsigned char value);
    unsigned char ppu_read(unsigned short address);
    void ppu_write(unsigned short address, unsigned char value);
    void reset();
    virtual void clock(int cycles);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 64,
                .name = "RAMBO-1",
                .constructor = []() { return std::make_unique<c_mapper64>(); },
            }
        };
    }
protected:
    virtual void fire_irq();
    void check_a12(int address);
    int chr[8];
    int prg[3];
    
    int reg_8000;
    int reg_8001;
    int reg_a000;
    int reg_c000;
    int reg_c001;
    int reg_e000;
    int reg_e001;

    void sync();

    int current_address;
    int low_count;
    void clock_irq_counter();

    unsigned char irq_counter;
    int irq_counter_reload;
    int irq_enabled;
    int irq_asserted;
    int irq_delay;
    int clock_scale;
    int ticks;

    int reload_cpu_counter;
    int cpu_divider;
    
    int irq_adjust;

    int next_reload;

    int a12_divider;

    int last_c000;

    int cycles_since_irq;
    int latched_value;
};

} //namespace nes
