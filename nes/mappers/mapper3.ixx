export module nes:mapper.mapper3;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

class c_mapper3 : public c_mapper, register_class<nes_mapper_registry, c_mapper3>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {{
            .number = 3,
            .name = "CNROM",
            .constructor = []() { return std::make_unique<c_mapper3>(); },
        }};
    }
    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            unsigned char existing_value = read_byte(address);

            if (existing_value != value) {
                int a = 1;
            }
            SetChrBank8k(value & existing_value);
        }
        else
            c_mapper::write_byte(address, value);
    }
};

} //namespace nes
