export module nes:mapper.mc_acc;
import :mapper;
import class_registry;

import :mapper.mapper4;

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
                .clock_rate = MAPPER_CLOCK_RATE::PPU,
                .constructor = []() { return std::make_unique<c_mapper_mc_acc>(); },
            },
        };
    }

    c_mapper_mc_acc()
    {
        a12_low_time = 35;
    }

    //MC-ACC fires IRQ on trailing edge of A12 and games are sensitive
    //to this timing.  Need to assert IRQ at the correct PPU cycle.
    //There's probably a more efficient way to do this...
    void ppu_clock() override
    {
        if (irq_delay > 0) {
            irq_delay -= 1;
            if (irq_delay <= 0) {
                c_mapper4::fire_irq();
            }
        }
    }

    void reset() override
    {
        irq_delay = 0;
        c_mapper4::reset();
    }

  protected:
    void fire_irq() override
    {
        irq_delay = 3;
    }

  private:
    int irq_delay;
};

} //namespace nes
