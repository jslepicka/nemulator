export module nes:mapper.mapper105;
import :mapper;

import :mapper.mapper1;

namespace nes
{

class c_mapper105 : public c_mapper1, register_class<nes_mapper_registry, c_mapper105>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 105,
                .name = "NWC",
                .constructor = []() { return std::make_unique<c_mapper105>(); },
            },
        };
    }

    void reset() override
    {
        irq_counter = 0;
        irq_asserted = 0;
        init = 0;
        ticks = 0;
    }

    void clock(int cycles) override
    {
        ticks += cycles;
        while (ticks > 2) {
            if (regs[1] & 0x10) {
                irq_counter = 0;
                if (irq_asserted) {
                    irq_asserted = 0;
                    clear_irq();
                }
            }
            else {
                irq_counter++;
                if (irq_counter == irq_trigger && !irq_asserted) {
                    execute_irq();
                    irq_asserted = 1;
                }
            }
            ticks -= 3;
        }
        c_mapper1::clock(cycles);
    }

    int get_nwc_time()
    {
        int remaining_ticks = irq_trigger - (irq_counter > irq_trigger ? irq_trigger : irq_counter);
        return remaining_ticks / 1786840;
    }

  private:
    int init;
    int irq_counter;
    int irq_asserted;
    int ticks;
    static const int irq_trigger = 0x28000000;
    
    void sync_prg()
    {
        switch (init) {
            case 0:
                if (regs[1] & 0x10)
                    init++;
                break;
            case 1:
                if (!(regs[1] & 0x10))
                    init++;
                break;
            default:
                break;
        }

        if (init >= 2) {
            if (regs[1] & 0x08) {
                if (regs[0] & 0x08) {
                    if (regs[0] & 0x4) {
                        SetPrgBank16k(PRG_8000, (regs[3] & 0x7) | 0x8);
                        SetPrgBank16k(PRG_C000, 0xF);
                    }
                    else {
                        SetPrgBank16k(PRG_8000, 0x8);
                        SetPrgBank16k(PRG_C000, (regs[3] & 0x7) & 0x8);
                    }
                }
                else {
                    SetPrgBank32k(((regs[3] >> 1) & 0x3) | 0x4);
                }
            }
            else {
                SetPrgBank32k((regs[1] >> 1) & 0x3);
            }
        }
        else {
            SetPrgBank32k(0);
        }
    }

};

} //namespace nes
