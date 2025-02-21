module;
#include "..\mirroring_types.h"
export module nes_mapper.mapper89;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

//Tenka no Goikenban - Mito Koumon (J)

class c_mapper89 : public c_mapper, register_class<nes_mapper_registry, c_mapper89>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 89,
                .constructor = []() { return std::make_unique<c_mapper89>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            if (value & 0x8)
                set_mirroring(MIRRORING_ONESCREEN_HIGH);
            else
                set_mirroring(MIRRORING_ONESCREEN_LOW);

            SetPrgBank16k(PRG_8000, (value >> 4) & 0x7);
            SetChrBank8k((value & 0x7) | ((value >> 4) & 0x8));
        }
    }

    void reset() override
    {
        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    }

};

} //namespace nes
