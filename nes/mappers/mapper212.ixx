module;

export module nes:mapper.mapper212;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

// 10000000-in-1 [p1][!].nes
// 1997-in-1 [p1][!].nes

class c_mapper212 : public c_mapper, register_class<nes_mapper_registry, c_mapper212>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {{
            .number = 212,
            .name = "BMC Super HiK 300-in-1",
            .constructor = []() { return std::make_unique<c_mapper212>(); },
        }};
    }

    void reset() override
    {
        set_mirroring(MIRRORING_VERTICAL);
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            if (address & 0x4000) {
                SetPrgBank32k((address >> 1) & 0x3);
            }
            else {
                SetPrgBank16k(PRG_8000, address & 0x7);
                SetPrgBank16k(PRG_C000, address & 0x7);
            }
            SetChrBank8k(address & 0x7);
            set_mirroring(address & 0x8 ? MIRRORING_HORIZONTAL : MIRRORING_VERTICAL);
        }
    }

    unsigned char read_byte(unsigned short address) override
    {
        if (address < 0x8000) {
            if ((address & 0xE010) == 0x6000) {
                return 0x80;
            }
        }
        return c_mapper::read_byte(address);
    }
};

} //namespace nes