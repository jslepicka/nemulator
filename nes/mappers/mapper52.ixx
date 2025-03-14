module;

export module nes:mapper.mapper52;
import :mapper.mapper4;
import class_registry;
import nemulator.std;

namespace nes
{
export class c_mapper52 : public c_mapper4, register_class<nes_mapper_registry, c_mapper52>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 52,
                .name = "Mario Party 7-in-1",
                .constructor = []() { return std::make_unique<c_mapper52>(); },
            },
        };
    }

    void reset() override
    {
        c_mapper4::reset();
        reg = 0;
        prg_mask = 0x1F;
        chr_mask = 0xFF;
        Sync();
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x6000 && address < 0x8000) {
            if (!writeProtectSram && sram_enabled) {
                if ((reg & 0x80) == 0) {
                    reg = value;
                    prg_offset = ((reg & 0x6) | (reg >> 3 & reg & 0x1)) << 4;
                    prg_mask = reg & 0x08 ? 0x0F : 0x1F;
                    chr_offset = ((reg >> 3 & 0x4) | (reg >> 1 & 0x2) | ((reg >> 6) & (reg >> 4) & 0x1)) << 7;
                    chr_mask = reg & 0x40 ? 0x7F : 0xFF;
                    Sync();
                }
                else {
                    c_mapper4::write_byte(address, value);
                }
            }
        }
        else {
            c_mapper4::write_byte(address, value);
        }
    }

  private:
    int reg;
};
} //namespace nes