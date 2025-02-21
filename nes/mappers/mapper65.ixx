module;
#include "..\mirroring_types.h"
export module nes_mapper.mapper65;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

//Daiku no Gen San 2
//Kaiketsu Yanchamaru 3
//Spartan X 2

class c_mapper65 : public c_mapper, register_class<nes_mapper_registry, c_mapper65>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 65,
                .constructor = []() { return std::make_unique<c_mapper65>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            switch (address >> 12) {
                case 0x8:
                    SetPrgBank8k(PRG_8000, value);
                    break;
                case 0x9:
                    switch (address & 0x7) {
                        case 1:
                            if (value & 0x80)
                                set_mirroring(MIRRORING_HORIZONTAL);
                            else
                                set_mirroring(MIRRORING_VERTICAL);
                            break;
                        case 3:
                            irq_enabled = value & 0x80;
                            if (irq_asserted) {
                                clear_irq();
                                irq_asserted = 0;
                            }
                            break;
                        case 4:
                            irq_counter = (irq_reload_high << 8) | irq_reload_low;
                            if (irq_asserted) {
                                clear_irq();
                                irq_asserted = 0;
                            }
                            break;
                        case 5:
                            irq_reload_high = value;
                            break;
                        case 6:
                            irq_reload_low = value;
                            break;
                    }
                    break;
                case 0xA:
                    SetPrgBank8k(PRG_A000, value);
                    break;
                case 0xB:
                    SetChrBank1k(address & 0x7, value);
                    break;
                case 0xC:
                    SetPrgBank8k(PRG_C000, value);
                    break;
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        irq_counter = 0;
        irq_reload_high = 0;
        irq_reload_low = 0;
        irq_enabled = 0;
        irq_asserted = 0;
        ticks = 0;
        SetPrgBank8k(PRG_8000, 0);
        SetPrgBank8k(PRG_A000, 1);
        SetPrgBank8k(PRG_C000, 0xFE);
        SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
    }

    void clock(int cycles) override
    {
        if (irq_enabled) {
            ticks += cycles;
            while (ticks > 2) {
                ticks -= 3;
                if (irq_counter > 0) {
                    irq_counter--;
                    if (irq_counter == 0) {
                        execute_irq();
                        irq_asserted = 1;
                    }
                }
            }
        }
    }

  private:
    int irq_enabled;
    int irq_asserted;
    int irq_counter;
    int irq_reload_high;
    int irq_reload_low;
    int ticks;
};

} //namespace nes
