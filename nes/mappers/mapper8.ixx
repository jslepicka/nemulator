module;

export module nes:mapper.mapper8;
import :mapper;
import class_registry;
import std;

namespace nes
{

class c_mapper8 : public c_mapper, register_class<nes_mapper_registry, c_mapper8>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 8,
                .name = "FFE F3xxx",
                .constructor = []() { return std::make_unique<c_mapper8>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            SetChrBank8k(value & 0x7);
            SetPrgBank16k(PRG_8000, value >> 3);
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        SetPrgBank16k(PRG_C000, 1);
    }

};

} //namespace nes
