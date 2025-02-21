module;
#include "..\mirroring_types.h"
export module nes_mapper.mapper78;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

class c_mapper78 : public c_mapper, register_class<nes_mapper_registry, c_mapper78>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 78,
                .constructor = []() { return std::make_unique<c_mapper78>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            SetPrgBank16k(PRG_8000, value & 0x7);
            SetChrBank8k(value >> 4);
            switch (mirror_mode) {
                case 0:
                    if (value & 0x8)
                        set_mirroring(MIRRORING_VERTICAL);
                    else
                        set_mirroring(MIRRORING_HORIZONTAL);
                    break;
                case 1:
                    if (value & 0x08)
                        set_mirroring(MIRRORING_ONESCREEN_HIGH);
                    else
                        set_mirroring(MIRRORING_ONESCREEN_LOW);
                    break;
            }
        }
        else

            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        switch (submapper) {
            case 1:
                mirror_mode = 0;
                break;
            case 2:
                mirror_mode = 1;
                break;
            default:
                break;
        }

        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    }

  private:
    int mirror_mode = 0;
};

} //namespace nes
