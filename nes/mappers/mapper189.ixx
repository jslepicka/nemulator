export module nes_mapper.mapper189;
import nes_mapper;

import nes_mapper.mapper4;

namespace nes
{

//Thunder Warrior, SF2 World Warrior

class c_mapper189 : public c_mapper4, register_class<nes_mapper_registry, c_mapper189>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 189,
                .constructor = []() { return std::make_unique<c_mapper189>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address < 0x8000 && address >= 0x4120) {
            if (address != 0x4132) {
                int a = 1;
            }
            reg_a = (value >> 4) | value;
        }
        else if (address >= 0x8000) {
            c_mapper4::write_byte(address, value);
        }
        Sync();
    }

    void reset() override
    {
        c_mapper4::reset();
        reg_a = 0;
        reg_b = 0;
        Sync();
    }

  private:
    int reg_a;
    int reg_b;

    void Sync()
    {
        c_mapper4::Sync();
        SetPrgBank32k(reg_a);
    }
};

} //namespace nes
