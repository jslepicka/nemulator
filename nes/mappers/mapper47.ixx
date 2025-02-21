export module nes_mapper.mapper47;
import nes_mapper;

import nes_mapper.mapper4;

namespace nes
{

//Super Spike V'Ball + Nintendo World Cup

class c_mapper47 : public c_mapper4, register_class<nes_mapper_registry, c_mapper47>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 47,
                .constructor = []() { return std::make_unique<c_mapper47>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        switch (address >> 12) {
            case 0x6:
            case 0x7:
                if (sram_enabled && !writeProtectSram) {
                    int block = value & 0x1;
                    prg_offset = block * 16;
                    chr_offset = block * 128;
                    Sync();
                }
                break;
            default:
                c_mapper4::write_byte(address, value);
                break;
        }
    }

    void reset() override
    {
        c_mapper4::reset();
        last_prg_page = 15;
        Sync();
    }
};

} //namespace nes
