module;
#include "..\mapper.h"
export module nes_mapper.mapper87;

namespace nes
{

//City Connection (J)

class c_mapper87 : public c_mapper, register_class<nes_mapper_registry, c_mapper87>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 87,
                .constructor = []() { return std::make_unique<c_mapper87>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        switch (address >> 12) {
            case 6:
            case 7:
                SetChrBank8k(((value & 0x1) << 1) | (value & 0x2) >> 1);
                break;
            default:
                c_mapper::write_byte(address, value);
        }
    }
};

} //namespace nes
