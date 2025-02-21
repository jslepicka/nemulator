module;
#include "..\mirroring_types.h"
export module nes_mapper.mapper70;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

//Family Trainer - Manhattan Police (J)
//Ge Ge Ge no Kitarou 2 (J)
//Kamen Rider Club (J)

class c_mapper70 : public c_mapper, register_class<nes_mapper_registry, c_mapper70>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 70,
                .constructor = []() { return std::make_unique<c_mapper70>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            SetPrgBank16k(PRG_8000, (value & 0x70) >> 4);
            SetChrBank8k(value & 0xF);
            if (value & 0x80)
                set_mirroring(MIRRORING_ONESCREEN_HIGH);
            else
                set_mirroring(MIRRORING_ONESCREEN_LOW);
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
