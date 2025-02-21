module;
#include "..\mapper.h"
export module nes_mapper.mapper152;

namespace nes
{

//Arkanoid II (J)

class c_mapper152 : public c_mapper, register_class<nes_mapper_registry, c_mapper152>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 152,
                .constructor = []() { return std::make_unique<c_mapper152>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            if (value & 0x80)
                set_mirroring(MIRRORING_ONESCREEN_LOW);
            else
                set_mirroring(MIRRORING_ONESCREEN_HIGH);
            SetPrgBank16k(PRG_8000, (value >> 4) & 0x07);
            SetChrBank8k(value & 0x0F);
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    }

};

} //namespace nes
