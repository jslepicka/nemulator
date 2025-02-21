module;
#include "..\mapper.h"
export module nes_mapper.mc_acc;

import nes_mapper.mapper4;

namespace nes
{

//Incredible Crash Dummies

class c_mapper_mc_acc : public c_mapper4, register_class<nes_mapper_registry, c_mapper_mc_acc>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 0x101,
                .name = "MC-ACC",
                .constructor = []() { return std::make_unique<c_mapper_mc_acc>(); },
            },
        };
    }

    void clock(int cycles) override
    {
        if (irq_delay > 0) {
            irq_delay -= cycles;
            if (irq_delay <= 0) {
                c_mapper4::fire_irq();
            }
        }
        if (!(current_address & 0x1000)) {
            low_count -= cycles;
            if (low_count < 0)
                low_count = 0;
        }
        else
            low_count = 35;
    }

    void reset() override
    {
        irq_delay = 0;
        c_mapper4::reset();
    }

  protected:
    void fire_irq()
    {
        irq_delay = 4;
    }

  private:
    int irq_delay;
};

} //namespace nes
