module;

export module nes:mapper.mapper93;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

//Fantasy Zone

class c_mapper93 : public c_mapper, register_class<nes_mapper_registry, c_mapper93>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 93,
                .constructor = []() { return std::make_unique<c_mapper93>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            if (value & 0x01)
                set_mirroring(MIRRORING_HORIZONTAL);
            else
                set_mirroring(MIRRORING_VERTICAL);
            SetPrgBank16k(PRG_8000, (value & 0xF0) >> 4);
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    }
};

} //namespace nes
