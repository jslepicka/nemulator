module;

export module nes:mapper.mapper193;
import :mapper;
import class_registry;
import std;

namespace nes
{

//Fighting Hero

class c_mapper193 : public c_mapper, register_class<nes_mapper_registry, c_mapper193>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 193,
                .constructor = []() { return std::make_unique<c_mapper193>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x6000 && address < 0x8000) {
            switch (address & 0x3) {
                case 0x0:
                    SetChrBank4k(CHR_0000, value >> 2);
                    break;
                case 0x1:
                    SetChrBank2k(CHR_1000, value >> 1);
                    break;
                case 0x2:
                    SetChrBank2k(CHR_1800, value >> 1);
                    break;
                case 0x3:
                    SetPrgBank8k(PRG_8000, value);
                    break;
            }
        }
        else {
            c_mapper::write_byte(address, value);
        }
    }

    void reset() override
    {
        SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
        SetPrgBank8k(PRG_C000, prgRomPageCount8k - 2);
        SetPrgBank8k(PRG_A000, prgRomPageCount8k - 3);
        SetPrgBank8k(PRG_8000, prgRomPageCount8k - 4);
        set_mirroring(MIRRORING_VERTICAL);
    }

};

} //namespace nes
