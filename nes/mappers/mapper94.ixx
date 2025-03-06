export module nes:mapper.mapper94;
import :mapper;
import class_registry;
import std;

namespace nes
{

//Senjou no Ookami

class c_mapper94 : public c_mapper, register_class<nes_mapper_registry, c_mapper94>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 94,
                .constructor = []() { return std::make_unique<c_mapper94>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            SetPrgBank16k(PRG_8000, (value & 0x1C) >> 2);
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
