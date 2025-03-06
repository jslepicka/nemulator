module;

export module nes:mapper.mapper7;
import :mapper;
import class_registry;
import std;

namespace nes
{

//Battletoads, Wizards and Warriors

class c_mapper7 : public c_mapper, register_class<nes_mapper_registry, c_mapper7>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {{
            .number = 7,
            .name = "AxROM",
            .constructor = []() { return std::make_unique<c_mapper7>(); },
        }};
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            SetPrgBank32k(value & 0xF);
            if (value & 0x10)
                set_mirroring(MIRRORING_ONESCREEN_HIGH);
            else
                set_mirroring(MIRRORING_ONESCREEN_LOW);
        }
        else
            c_mapper::write_byte(address, value);
    }
    void reset() override
    {
        set_mirroring(MIRRORING_ONESCREEN_LOW);
        SetPrgBank32k(0);
    }
};

} //namespace nes
