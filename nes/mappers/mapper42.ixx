module;
#include "..\mapper.h"
export module nes_mapper.mapper42;

namespace nes
{

//Bio Miracle Bokutte Upa (J) (Mario Baby - FDS Conversion)

class c_mapper42 : public c_mapper, register_class<nes_mapper_registry, c_mapper42>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 42,
                .constructor = []() { return std::make_unique<c_mapper42>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0xE000) {
            switch (address & 0x3) {
                case 0:
                    prg_6k = pPrgRom + ((value % prgRomPageCount8k) * 0x2000);
                    break;
                case 1:
                    if (value & 0x8)
                        set_mirroring(MIRRORING_HORIZONTAL);
                    else
                        set_mirroring(MIRRORING_VERTICAL);
                    break;
                case 2:
                    irq_enabled = value & 0x2;
                    if (!irq_enabled) {
                        irq_counter = 0;
                        if (irq_asserted) {
                            clear_irq();
                            irq_asserted = 0;
                        }
                    }
                    break;
                case 3:
                    break;
            }
        }
    }

    void reset() override
    {
        ticks = 0;
        irq_enabled = 0;
        irq_asserted = 0;
        irq_counter = 0;
        prg_6k = pPrgRom;
        SetPrgBank32k(prgRomPageCount32k - 1);
    }

    void clock(int cycles) override
    {
        if (irq_enabled) {
            ticks += cycles;
            while (ticks > 2) {
                ticks -= 3;
                if (++irq_counter == 0x6000 && !irq_asserted) {
                    execute_irq();
                    irq_asserted = 1;
                }
            }
        }
    }

    unsigned char read_byte(unsigned short address) override
    {
        if (address >= 0x6000 && address < 0x8000) {
            return *(prg_6k + (address & 0x1FFF));
        }
        else
            return c_mapper::read_byte(address);
    }

  private:
    int irq_enabled;
    int irq_asserted;
    int irq_counter;
    int ticks;
    unsigned char *prg_6k;
};

} //namespace nes
