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
        switch (address >> 12) {
            case 6:
            case 7:
                if (!writeProtectSram && sram_enabled) {
                    if ((reg & 0x80) == 0) {
                        reg = value;
                        prg_offset = reg & 0x6;
                        if (reg & 0x08) {
                            prg_mask = 0x0F;
                            prg_offset |= (reg & 0x01);
                        }
                        else {
                            prg_mask = 0x1F;
                        }
                        prg_offset <<= 4;

                        chr_offset = ((reg & 0x20) >> 3) | ((reg & 0x04) >> 1);
                        if (reg & 0x40) {
                            chr_mask = 0x7F;
                            chr_offset |= (reg & 0x10) >> 4;
                        }
                        else {
                            chr_mask = 0xFF;
                        }
                        chr_offset <<= 7;

                        Sync();
                    }
                    else {
                        c_mapper4::write_byte(address, value);
                    }
                }
                break;
            default:
                c_mapper4::write_byte(address, value);
                break;
        }
    }

  private:
    int reg;
};
} //namespace nes