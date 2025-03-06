export module nes:mapper.mapper92;
import :mapper;
import class_registry;
import std;

namespace nes
{

//Moero!! Pro Soccer (J)
//Moero!! Pro Yakuu '88 - Ketteiban (J)

class c_mapper92 : public c_mapper, register_class<nes_mapper_registry, c_mapper92>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 92,
                .constructor = []() { return std::make_unique<c_mapper92>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            switch ((value >> 6) & 0x3) {
                case 0:
                    if (latch & 0x40) {
                        SetChrBank8k(latch & 0xF);
                    }
                    if (latch & 0x80) {
                        SetPrgBank16k(PRG_C000, latch & 0xF);
                    }
                    break;
                case 1:
                case 2:
                    latch = value;
                    break;
                case 3:
                    break;
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        latch = 0;
        SetPrgBank16k(PRG_8000, 0);
    }

  private:
    int latch;
};

} //namespace nes
