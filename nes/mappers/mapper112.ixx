module;

export module nes:mapper.mapper112;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

class c_mapper112 : public c_mapper, register_class<nes_mapper_registry, c_mapper112>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 112,
                .constructor = []() { return std::make_unique<c_mapper112>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            switch (address & 0xE001) {
                case 0x8000:
                    command = value & 0x7;
                    break;
                case 0x8001: {
                    int x = 1;
                } break;
                case 0xA000:
                    switch (command) {
                        case 0:
                        case 1:
                            SetPrgBank8k(command, value);
                            break;
                        case 2:
                            SetChrBank2k(CHR_0000, value >> 1);
                            break;
                        case 3:
                            SetChrBank2k(CHR_0800, value >> 1);
                            break;
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                            SetChrBank1k(command, value);
                            break;
                    }
                    break;
                case 0xE000:
                    if (value & 0x1)
                        set_mirroring(MIRRORING_HORIZONTAL);
                    else
                        set_mirroring(MIRRORING_VERTICAL);
                    break;
            }
        }
    }

    void reset() override
    {
        command = 0;
        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    }


  private:
    int command;
};

} //namespace nes
