module;

export module nes:mapper.mapper71;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

//Camerica games: Bee 52, Mig 29, etc.

class c_mapper71 : public c_mapper, register_class<nes_mapper_registry, c_mapper71>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 71,
                .constructor = []() { return std::make_unique<c_mapper71>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0xC000) {
            SetPrgBank16k(PRG_8000, value);
        }
        else if (enable_mirroring_control && address >= 0x8000 && address <= 0x9FFF) {
            if (value & 0x10)
                set_mirroring(MIRRORING_ONESCREEN_HIGH);
            else
                set_mirroring(MIRRORING_ONESCREEN_LOW);
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        enable_mirroring_control = 0;
        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
        if (submapper == 1) {
            enable_mirroring_control = 1;
        }
    }

  private:
    int enable_mirroring_control = 1;

};

} //namespace nes
