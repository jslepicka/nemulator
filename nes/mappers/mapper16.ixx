module;

export module nes:mapper.mapper16;
import :mapper;
import class_registry;
import std;

namespace nes
{

//Dragon Ball Z, etc.
//No EPROM support

class c_mapper16 : public c_mapper, register_class<nes_mapper_registry, c_mapper16>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 16,
                .constructor = []() { return std::make_unique<c_mapper16>(); },
            },
            {
                .number = 159,
                .constructor = []() { return std::make_unique<c_mapper16>(1); },
            },
        };
    }

    c_mapper16(int submapper = 0)
    {
        this->submapper = submapper;
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x6000) {
            switch (address & 0xF) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                    SetChrBank1k(address & 0xF, value);
                    break;
                case 8:
                    SetPrgBank16k(PRG_8000, value);
                    break;
                case 9:
                    switch (value & 0x3) {
                        case 0:
                            set_mirroring(MIRRORING_VERTICAL);
                            break;
                        case 1:
                            set_mirroring(MIRRORING_HORIZONTAL);
                            break;
                        case 2:
                            set_mirroring(MIRRORING_ONESCREEN_LOW);
                            break;
                        case 3:
                            set_mirroring(MIRRORING_ONESCREEN_HIGH);
                            break;
                    }
                    break;
                case 0xA:
                    irq_enabled = value & 0x1;
                    if (irq_asserted) {
                        clear_irq();
                        irq_asserted = 0;
                    }
                    break;
                case 0xB:
                    irq_counter = (irq_counter & 0xFF00) | value;
                    break;
                case 0xC:
                    irq_counter = (irq_counter & 0xFF) | (value << 8);
                    break;
                case 0xD:
                    break;
                default:
                    break;
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
        irq_counter = 0;
        irq_enabled = 0;
        irq_asserted = 0;
        ticks = 0;
    }

    void clock(int cycles) override
    {
        ticks += cycles;
        while (ticks > 2) {
            ticks -= 3;
            if (irq_enabled) {
                int prev_counter = irq_counter;
                irq_counter--;

                if (prev_counter == 1 && irq_counter == 0) {
                    if (!irq_asserted) {
                        execute_irq();
                        irq_asserted = 1;
                    }
                }
            }
        }
    }

  private:
    unsigned char *eprom;
    int irq_counter;
    int irq_enabled;
    int irq_asserted;
    int ticks;
};

} //namespace nes
