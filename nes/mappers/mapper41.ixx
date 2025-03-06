module;

export module nes:mapper.mapper41;
import :mapper;
import class_registry;
import std;

namespace nes
{

class c_mapper41 : public c_mapper, register_class<nes_mapper_registry, c_mapper41>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 41,
                .name = "Caltron 6-in-1",
                .constructor = []() { return std::make_unique<c_mapper41>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            if (enable_reg2) {
                chr = (chr & 0xC) | (value & 0x3);
                SetChrBank8k(chr);
            }
        }
        else if (address >= 0x6000 && address < 0x6800) {
            enable_reg2 = address & 0x4;
            SetPrgBank32k(address & 0x7);
            if (address & 0x20)
                set_mirroring(MIRRORING_HORIZONTAL);
            else
                set_mirroring(MIRRORING_VERTICAL);
            chr = (chr & 0x3) | ((address >> 1) & 0xC);
            SetChrBank8k(chr);
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        SetPrgBank32k(0);
    }

  private:
    int enable_reg2;
    int chr;
};

} //namespace nes
