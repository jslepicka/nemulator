export module nes:mapper.mapper140;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

//Bio Senshi Dan - Increaser Tono Tatakai

class c_mapper140 : public c_mapper, register_class<nes_mapper_registry, c_mapper140>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 140,
                .constructor = []() { return std::make_unique<c_mapper140>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        switch (address >> 12) {
            case 6:
            case 7:
                SetChrBank8k(value & 0xF);
                SetPrgBank32k((value >> 4) & 0x3);
                break;
            default:
                c_mapper::write_byte(address, value);
        }
    }

};

} //namespace nes
