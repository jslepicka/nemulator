module;

export module nes:mapper.mapper113;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

//Rad Racket, Papillon

class c_mapper113 : public c_mapper, register_class<nes_mapper_registry, c_mapper113>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 113,
                .constructor = []() { return std::make_unique<c_mapper113>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if ((address & 0x4100) >= 0x4100 && (address & 0x4100) < 0x6000) {
            if (value & 0x80)
                set_mirroring(MIRRORING_VERTICAL);
            else
                set_mirroring(MIRRORING_HORIZONTAL);
            SetPrgBank32k((value >> 3) & 0x7);
            SetChrBank8k((value & 0x7) | ((value >> 3) & 0x8));
        }
        else
            c_mapper::write_byte(address, value);
    }
};

} //namespace nes
