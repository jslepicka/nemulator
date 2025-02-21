module;
#include "..\mapper.h"
export module nes_mapper.mapper115;

import nes_mapper.mapper4;

namespace nes
{

//AV Jiu Ji Ma Jiang 2 (Unl)
//Yuu Yuu Hakusho Final - Makai Saikyou Retsuden (English) (Unl)

class c_mapper115 : public c_mapper4, register_class<nes_mapper_registry, c_mapper115>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 115,
                .constructor = []() { return std::make_unique<c_mapper115>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        switch (address >> 12) {
            case 0x6:
            case 0x7:
                if (address & 0x1) {
                    chr_offset = (value & 0x1) << 8;
                }
                else {
                    reg1 = value;
                }
                Sync();
                break;
            default:
                c_mapper4::write_byte(address, value);
                break;
        }
    }

  private:
    int reg1;

    void reset()
    {
        reg1 = 0;
        c_mapper4::reset();
    }

    void Sync()
    {
        c_mapper4::Sync();
        if (reg1 & 0x80) {
            SetPrgBank16k(PRG_8000, reg1 & 0xF);
        }
    }
};

} //namespace nes
