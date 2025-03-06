module;

export module nes:mapper.mapper202;
import :mapper;
import class_registry;
import std;

namespace nes
{

// 150-in-1 (Mapper 202) [p1][!].nes

class c_mapper202 : public c_mapper, register_class<nes_mapper_registry, c_mapper202>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {{
            .number = 202,
            .name = "150-in-1",
            .constructor = []() { return std::make_unique<c_mapper202>(); },
        }};
    }

    void reset() override
    {
        set_mirroring(MIRRORING_VERTICAL);
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            if ((address & 0x9) == 0x9) {
                SetPrgBank32k((address >> 2) & 0x3);
            }
            else {
                SetPrgBank16k(PRG_8000, (address >> 1) & 0x7);
                SetPrgBank16k(PRG_C000, (address >> 1) & 0x7);
            }
            SetChrBank8k((address >> 1) & 0x7);
            set_mirroring(address & 0x1 ? MIRRORING_HORIZONTAL : MIRRORING_VERTICAL);
        }
    }
};

} //namespace nes