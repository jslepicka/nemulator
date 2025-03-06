export module nes:mapper.mapper66;
import :mapper;
import class_registry;
import std;

namespace nes
{

//SMB + Duckhunt

class c_mapper66 : public c_mapper, register_class<nes_mapper_registry, c_mapper66>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 66,
                .name = "GxROM",
                .constructor = []() { return std::make_unique<c_mapper66>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            SetPrgBank32k((value & 0x30) >> 4);
            SetChrBank8k(value & 0x03);
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        SetPrgBank32k(0);
        SetChrBank8k(0);
    }
};

} //namespace nes
