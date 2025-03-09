export module nes:mapper.mapper146;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

//Sachen
//Master Chu & The Drunkard Hu

class c_mapper146 : public c_mapper, register_class<nes_mapper_registry, c_mapper146>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 146,
                .constructor = []() { return std::make_unique<c_mapper146>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x4100 && address < 0x6000) {
            if (address & 0x100) {
                SetPrgBank32k(value >> 3);
                SetChrBank8k(value);
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

};

} //namespace nes
