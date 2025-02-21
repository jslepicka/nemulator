module;
#include "..\mirroring_types.h"
export module nes_mapper.mapper33;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

//Bubble Bobble 2 (J)

class c_mapper33 : public c_mapper, register_class<nes_mapper_registry, c_mapper33>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 33,
                .constructor = []() { return std::make_unique<c_mapper33>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            switch (address & 0xA003) {
                case 0x8000:
                    if (value & 0x40)
                        set_mirroring(MIRRORING_HORIZONTAL);
                    else
                        set_mirroring(MIRRORING_VERTICAL);
                    SetPrgBank8k(PRG_8000, value & 0x3F);
                    break;
                case 0x8001:
                    SetPrgBank8k(PRG_A000, value & 0x3F);
                    break;
                case 0x8002:
                    SetChrBank2k(CHR_0000, value);
                    break;
                case 0x8003:
                    SetChrBank2k(CHR_0800, value);
                    break;
                case 0xA000:
                    SetChrBank1k(CHR_1000, value);
                    break;
                case 0xA001:
                    SetChrBank1k(CHR_1400, value);
                    break;
                case 0xA002:
                    SetChrBank1k(CHR_1800, value);
                    break;
                case 0xA003:
                    SetChrBank1k(CHR_1C00, value);
                    break;
            }
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
