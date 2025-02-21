module;
#include "..\mirroring_types.h"
export module nes_mapper.mapper80;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

class c_mapper80 : public c_mapper, register_class<nes_mapper_registry, c_mapper80>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 80,
                .constructor = []() { return std::make_unique<c_mapper80>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x7000 && address < 0x8000) {
            switch (address & 0x7EFF) {
                case 0x7EF0:
                    SetChrBank2k(CHR_0000, value >> 1);
                    break;
                case 0x7EF1:
                    SetChrBank2k(CHR_0800, value >> 1);
                    break;
                case 0x7EF2:
                case 0x7EF3:
                case 0x7EF4:
                case 0x7EF5:
                    SetChrBank1k(address - 0x7EEE, value);
                    break;
                case 0x7EF6:
                    if (value & 0x1)
                        set_mirroring(MIRRORING_VERTICAL);
                    else
                        set_mirroring(MIRRORING_HORIZONTAL);
                    break;
                case 0x7EFA:
                case 0x7EFB:
                    SetPrgBank8k(PRG_8000, value);
                    break;
                case 0x7EFC:
                case 0x7EFD:
                    SetPrgBank8k(PRG_A000, value);
                    break;
                case 0x7EFE:
                case 0x7EFF:
                    SetPrgBank8k(PRG_C000, value);
                    break;
                    break;
                default:
                    break;
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
    }
};

} //namespace nes
