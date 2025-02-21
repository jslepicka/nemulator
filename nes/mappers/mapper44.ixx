export module nes_mapper.mapper44;
import nes_mapper;

import nes_mapper.mapper4;

namespace nes
{

//Super Cool Boy 4-in-1
//Super Big 7-in-1

class c_mapper44 : public c_mapper4, register_class<nes_mapper_registry, c_mapper44>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 44,
                .constructor = []() { return std::make_unique<c_mapper44>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if ((address & 0xE001) == 0xA001) {
            writeProtectSram = value & 0x40 ? true : false;
            sram_enabled = value & 0x80;

            int block = value & 0x7;

            if (block > 5) {
                prg_mask = 0x1F;
                chr_mask = 0xFF;
                prg_offset = 0x60;
                chr_offset = 0x300;
            }
            else {
                prg_mask = 0x0F;
                chr_mask = 0x7F;
                prg_offset = 0x10 * block;
                chr_offset = 0x80 * block;
            }
            Sync();
        }
        else {
            c_mapper4::write_byte(address, value);
        }
    }

    void reset() override
    {
        c_mapper4::reset();
        prg_mask = 0x0F;
        chr_mask = 0x7F;
        Sync();
    }
};

} //namespace nes
