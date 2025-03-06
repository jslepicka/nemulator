export module nes:mapper.mapper232;
import :mapper;
import class_registry;
import std;

namespace nes
{

//Quattro *

class c_mapper232 : public c_mapper, register_class<nes_mapper_registry, c_mapper232>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 232,
                .constructor = []() { return std::make_unique<c_mapper232>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        switch (address & 0xC000) {
            case 0x8000:
                bank = (value & 0x18) >> 1;
                sync();
                break;
            case 0xC000:
                page = value & 0x3;
                sync();
                break;
            default:
                c_mapper::write_byte(address, value);
                break;
        }
    }

    void reset() override
    {
        bank = 0;
        page = 0;
        sync();
    }

  private:
    int bank;
    int page;
    void sync()
    {
        SetPrgBank16k(PRG_8000, bank | page);
        SetPrgBank16k(PRG_C000, bank | 3);
    }
};

} //namespace nes
