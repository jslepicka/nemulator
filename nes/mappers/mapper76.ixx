module;

export module nes:mapper.mapper76;
import :mapper;
import class_registry;
import std;

namespace nes
{

//Digital Devil Monogatari (J)

class c_mapper76 : public c_mapper, register_class<nes_mapper_registry, c_mapper76>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 76,
                .constructor = []() { return std::make_unique<c_mapper76>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        switch (address & 0xE001) {
            case 0x8000:
                prg_mode = value & 0x40;
                command = value & 0x7;
                break;
            case 0x8001:
                switch (command) {
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                        SetChrBank2k((command - 2) * 2, value);
                        break;
                    case 6:
                        if (prg_mode)
                            SetPrgBank8k(PRG_C000, value);
                        else
                            SetPrgBank8k(PRG_8000, value);
                        break;
                    case 7:
                        SetPrgBank8k(PRG_A000, value);
                        break;
                    default:
                        break;
                }
                break;
            case 0xA000:
                if (value & 0x01)
                    set_mirroring(MIRRORING_HORIZONTAL);
                else
                    set_mirroring(MIRRORING_VERTICAL);
                break;
            default:
                c_mapper::write_byte(address, value);
        }
    }

    void reset() override
    {
        prg_mode = 0;
        command = 0;
        SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
        SetPrgBank8k(PRG_C000, 0xFE);
    }

  private:
    int prg_mode;
    int command;
};

} //namespace nes
