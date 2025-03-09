export module nes:mapper.mapper2;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

class c_mapper2 : public c_mapper, register_class<nes_mapper_registry, c_mapper2>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {{
            .number = 2,
            .name = "UxROM",
            .constructor = []() { return std::make_unique<c_mapper2>(); },
        }};
    }
    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000)
            SetPrgBank16k(PRG_8000, value);
        else
            c_mapper::write_byte(address, value);
    }
};

} //namespace nes
