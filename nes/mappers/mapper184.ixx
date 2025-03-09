export module nes:mapper.mapper184;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

//Atlantis no Nazo

class c_mapper184 : public c_mapper, register_class<nes_mapper_registry, c_mapper184>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 184,
                .constructor = []() { return std::make_unique<c_mapper184>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        switch (address >> 12) {
            case 6:
            case 7:
                SetChrBank4k(CHR_0000, value & 0x07);
                SetChrBank4k(CHR_1000, (value >> 4) & 0x07);
                break;
            default:
                c_mapper::write_byte(address, value);
                break;
        }
    }
};

} //namespace nes
