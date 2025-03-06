export module nes:mapper.mapper10;
import :mapper;
import :mapper.mapper9;

//Fire Emblem

namespace nes
{

class c_mapper10 : public c_mapper9, register_class<nes_mapper_registry, c_mapper10>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 10,
                .name = "MMC4",
                .constructor = []() { return std::make_unique<c_mapper10>(); },
            },
        };
    }
    
    void reset() override
    {
        c_mapper9::reset();
        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >> 12 == 0xA) {
            SetPrgBank16k(PRG_8000, value & 0xF);
        }
        else
            c_mapper9::write_byte(address, value);
    }

};

} //namespace nes
