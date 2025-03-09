export module nes:mapper.mapper73;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

class c_mapper73 : public c_mapper, register_class<nes_mapper_registry, c_mapper73>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 73,
                .name = "VRC3",
                .constructor = []() { return std::make_unique<c_mapper73>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            switch (address & 0xF000) {
                case 0x8000:
                case 0x9000:
                case 0xA000:
                case 0xB000: {
                    int x = ((address >> 12) & 0x03) * 4;
                    int mask = ~(0xF << x);
                    irq_reload = (irq_reload & mask) | ((value & 0xF) << x);
                } break;
                case 0xC000:
                    irq_mode = value & 0x4;
                    irq_enabled = value & 0x2;
                    irq_enable_on_ack = value & 0x1;
                    if (irq_enabled)
                        irq_counter = irq_reload;
                    ack_irq();
                    break;
                case 0xD000:
                    if (irq_enable_on_ack)
                        irq_enabled = 0x2;
                    else
                        irq_enabled = 0;
                    ack_irq();
                    break;
                case 0xF000:
                    SetPrgBank16k(PRG_8000, value & 0xF);
                    break;
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        irq_counter = 0;
        irq_reload = 0;
        irq_mode = 0;
        irq_enabled = 0;
        irq_enable_on_ack = 0;
        irq_asserted = 0;
        ticks = 0;
        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    }

    void clock(int cycles) override
    {
        if (irq_enabled) {
            ticks += cycles;
            while (ticks > 2) {
                ticks -= 3;
                if (irq_mode) //8-bit
                {
                    int temp = irq_counter & 0xFF;
                    temp = ((temp + 1) & 0xFF);
                    irq_counter = (irq_counter & 0xFF00) | temp;
                    if (temp == 0) {
                        if (!irq_asserted) {
                            execute_irq();
                            irq_asserted = 1;
                        }
                    }
                }
                else //16-bit
                {
                    irq_counter = ((irq_counter + 1) & 0xFFFF);
                    if (irq_counter == 0) {
                        if (!irq_asserted) {
                            execute_irq();
                            irq_asserted = 1;
                        }
                    }
                }
            }
        }
    }

  private:
    int irq_counter;
    int irq_reload;
    int irq_mode;
    int irq_enabled;
    int irq_enable_on_ack;
    int irq_asserted;
    int ticks;

    void ack_irq()
    {
        if (irq_asserted) {
            clear_irq();
            irq_asserted = 0;
        }
    }
};

} //namespace nes
