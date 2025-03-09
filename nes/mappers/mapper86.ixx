export module nes:mapper.mapper86;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

//Moero!! Pro Yakyuu (Red) (J)
//Moero!! Pro Yakyuu (Black) (J)

class c_mapper86 : public c_mapper, register_class<nes_mapper_registry, c_mapper86>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 86,
                .constructor = []() { return std::make_unique<c_mapper86>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        switch (address >> 12) {
            case 6:
                SetPrgBank32k((value >> 4) & 0x3);
                SetChrBank8k((value & 0x3) | ((value >> 4) & 0x4));
                break;
            case 7:
                break;
            default:
                c_mapper::write_byte(address, value);
        }
    }

};

} //namespace nes
