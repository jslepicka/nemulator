export module nes_mapper.mapper34;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

class c_mapper34 : public c_mapper, register_class<nes_mapper_registry, c_mapper34>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 34,
                .name = "BxROM",
                .constructor = []() { return std::make_unique<c_mapper34>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            SetPrgBank32k(value);
        }
        else
            c_mapper::write_byte(address, value);
    }
};

} //namespace nes
