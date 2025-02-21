module;
#include "..\mapper.h"
export module nes_mapper.mapper11;

namespace nes
{

class c_mapper11 : public c_mapper, register_class<nes_mapper_registry, c_mapper11>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 11,
                .name = "Color Dreams",
                .constructor = []() { return std::make_unique<c_mapper11>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            SetPrgBank32k(value & 0x3);
            SetChrBank8k(value >> 4);
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        SetPrgBank32k(0);
    }
};

} //namespace nes
