export module nes:mapper.mapper79;
import :mapper;
import class_registry;
import std;

namespace nes
{

//Krazy Kreatures

class c_mapper79 : public c_mapper, register_class<nes_mapper_registry, c_mapper79>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 79,
                .constructor = []() { return std::make_unique<c_mapper79>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address < 0x6000 && (address & 0x4100) == 0x4100) {
            SetChrBank8k((value & 0x07) | ((value & 0x40) >> 3));
            SetPrgBank32k((value >> 3) & 0x07);
        }
        else
            c_mapper::write_byte(address, value);
    }
};

} //namespace nes
