export module nes:mapper.mapper180;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

//Crazy Climber

class c_mapper180 : public c_mapper, register_class<nes_mapper_registry, c_mapper180>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 180,
                .constructor = []() { return std::make_unique<c_mapper180>(); },
            },
        };
    }

    void reset() override
    {
        SetPrgBank16k(PRG_8000, 0);
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            SetPrgBank16k(PRG_C000, value);
        }
        else {
            c_mapper::write_byte(address, value);
        }
    }

};

} //namespace nes
