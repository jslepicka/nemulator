export module nes:mapper.mapper40;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

class c_mapper40 : public c_mapper, register_class<nes_mapper_registry, c_mapper40>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 40,
                .constructor = []() { return std::make_unique<c_mapper40>(); },
            },
        };
    }
    
    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            switch (address >> 12) {
                case 8:
                case 9:
                    irq_enabled = 0;
                    irq_counter = 0;
                    clear_irq();
                    break;
                case 0xA:
                case 0xB:
                    irq_enabled = 1;
                    break;
                case 0xC:
                case 0xD:
                    break;
                case 0xE:
                case 0xF:
                    SetPrgBank8k(PRG_C000, value & 0x7);
                    break;
                default:
                    break;
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

    unsigned char read_byte(unsigned short address) override
    {
        if (address >= 0x6000 && address < 0x8000) {
            return *((pPrgRom + 0x2000 * 6) + (address & 0x1FFF));
        }
        return *(prgBank[(address >> 13) & 3] + (address & 0x1FFF));
    }

    void reset() override
    {
        irq_counter = 0;
        irq_enabled = 0;
        ticks = 0;
        SetPrgBank8k(PRG_8000, 4);
        SetPrgBank8k(PRG_A000, 5);
        SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
    }

    void clock(int cycles) override
    {
        if (irq_enabled) {
            ticks += cycles;
            while (ticks > 2) {
                if (++irq_counter == 4096) {
                    execute_irq();
                    irq_counter = 0;
                }
                ticks -= 3;
            }
        }
    }

  private:
    int irq_counter;
    int irq_enabled;
    int ticks;
};

} //namespace nes
