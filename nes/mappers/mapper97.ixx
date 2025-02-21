module;
#include "..\mirroring_types.h"
export module nes_mapper.mapper97;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

//Kaiketsu Yanchamaru (J)

class c_mapper97 : public c_mapper, register_class<nes_mapper_registry, c_mapper97>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 97,
                .constructor = []() { return std::make_unique<c_mapper97>(); },
            },
        };
    }

    void reset() override
    {
        SetPrgBank16k(PRG_8000, prgRomPageCount16k - 1);
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            SetPrgBank16k(PRG_C000, value & 0xF);
            switch (value >> 6) {
                case 0:
                    set_mirroring(MIRRORING_ONESCREEN_LOW);
                    break;
                case 1:
                    set_mirroring(MIRRORING_HORIZONTAL);
                    break;
                case 2:
                    set_mirroring(MIRRORING_VERTICAL);
                    break;
                case 3:
                    set_mirroring(MIRRORING_ONESCREEN_HIGH);
                    break;
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

};

} //namespace nes
