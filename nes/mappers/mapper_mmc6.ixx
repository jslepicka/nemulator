export module nes:mapper.mmc6;
import :mapper;
import class_registry;

import :mapper.mapper4;

namespace nes
{

//Startropics

class c_mapper_mmc6 : public c_mapper4, register_class<nes_mapper_registry, c_mapper_mmc6>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 0x100,
                .name = "MMC6",
                .constructor = []() { return std::make_unique<c_mapper_mmc6>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            switch (address & 0xE001) {
                case 0x8000:
                    commandNumber = value & 0x07;
                    prgSelect = value & 0x40 ? true : false;
                    prg_xor = prgSelect << 1;
                    chrXor = value & 0x80 ? true : false;
                    chr_xor = chrXor << 2;
                    wram_enable = value & 0x20;
                    if (!wram_enable)
                        wram_control = 0;
                    Sync();
                    break;
                case 0xA001:
                    if (wram_enable)
                        wram_control = value;
                    break;
                default:
                    c_mapper4::write_byte(address, value);
                    break;
            }
        }
        else if (address >= 0x7000) {
            address &= 0x13FF;
            if (((address & 0x200) && (wram_control & 0xC0)) || (!(address & 0x200) && (wram_control & 0x30)))
                sram[address] = value;
        }
    }

    unsigned char read_byte(unsigned short address) override
    {
        switch (address >> 12) {
            case 6:
                return 0;
            case 7:
                address &= 0x13FF;
                if (((address & 0x200) && (wram_control & 0x80)) || (!(address & 0x200) && (wram_control & 0x20)))
                    return sram[address];
                else
                    return 0;
            default:
                return c_mapper4::read_byte(address);
        }
    }

    void reset() override
    {
        wram_enable = 0;
        wram_control = 0;
        c_mapper4::reset();
    }

    int load_image() override
    {
        header->Rcb1.Sram = true;
        return c_mapper::load_image();
    }


  protected:
    int wram_enable;
    int wram_control;
};

} //namespace nes
