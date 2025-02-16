#pragma once
#include "mapper4.h"

namespace nes {

class c_mapper_mc_acc : public c_mapper4, register_mapper<c_mapper_mc_acc>
{
public:
    c_mapper_mc_acc();
    ~c_mapper_mc_acc();
    void clock(int cycles);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 0x101,
                .constructor = []() { return std::make_unique<c_mapper_mc_acc>(); },
            },
        };
    }

  protected:
    void fire_irq();
private:
    int irq_delay;
};

} //namespace nes
