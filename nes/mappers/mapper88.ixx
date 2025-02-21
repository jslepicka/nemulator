module;
#include "..\mirroring_types.h"
export module nes_mapper.mapper88;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

//Quinty (J)
//Devil Man (J)

class c_mapper88 : public c_mapper, register_class<nes_mapper_registry, c_mapper88>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 88,
                .constructor = []() { return std::make_unique<c_mapper88>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            if (address & 0x01) {
                switch (command) {
                    case 0:
                        SetChrBank2k(CHR_0000, (value & 0x3F) >> 1);
                        break;
                    case 1:
                        SetChrBank2k(CHR_0800, (value & 0x3F) >> 1);
                        break;
                    case 2:
                        SetChrBank1k(CHR_1000, (value & 0x3F) | 0x40);
                        break;
                    case 3:
                        SetChrBank1k(CHR_1400, (value & 0x3F) | 0x40);
                        break;
                    case 4:
                        SetChrBank1k(CHR_1800, (value & 0x3F) | 0x40);
                        break;
                    case 5:
                        SetChrBank1k(CHR_1C00, (value & 0x3F) | 0x40);
                        break;
                    case 6:
                        SetPrgBank8k(PRG_8000, value & 0x0F);
                        break;
                    case 7:
                        SetPrgBank8k(PRG_A000, value & 0x0F);
                        break;
                }
            }
            else {
                command = value & 0x7;
                if (submapper == 1) {
                    if (value & 0x40)
                        set_mirroring(MIRRORING_ONESCREEN_LOW);
                    else
                        set_mirroring(MIRRORING_ONESCREEN_HIGH);
                }
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
        command = 0;

        for (int i = CHR_0000; i <= CHR_1C00; i++)
            SetChrBank1k(i, 0);
    }

  private:
    unsigned char command;
};

} //namespace nes
