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
                .clock_rate = MAPPER_CLOCK_RATE::NONE,
                .constructor = []() { return std::make_unique<c_mapper_mc_acc>(); },
            },
        };
    }

    c_mapper_mc_acc()
    {
        a12_low_time = 35;
    }

    virtual void check_a12(int address) override
    {
        //on A12 transition
        if ((address ^ last_address) & 0x1000) {
            //if A12 is high and the last transition was >= 35 ppu
            //cycles ago, clock irq
            //TODO: document why delay here is more than MMC3
            if (address & 0x1000) {
                if (*ppu_cycle - last_a12_transition >= a12_low_time) {
                    clock_irq_counter();
                }
            }
            else {
                if (irq_pending) {
                    c_mapper4::fire_irq();
                    irq_pending = false;
                }
            }
            last_a12_transition = *ppu_cycle;
        }
        last_address = address;
    }

    void reset() override
    {
        irq_pending = false;
        c_mapper4::reset();
    }

  protected:
    void fire_irq() override
    {
        //MC-ACC IRQ is asserted on next hi->low A12 transition so
        //just mark it as pending here
        irq_pending = true;
    }

  private:
    bool irq_pending;
};

} //namespace nes
