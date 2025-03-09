module;

#include <memory>
export module nes:mapper.mapper190;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

//Magic Kid GooGoo

class c_mapper190 : public c_mapper, register_class<nes_mapper_registry, c_mapper190>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 190,
                .constructor = []() { return std::make_unique<c_mapper190>(); },
            },
        };
    }

    c_mapper190()
    {
        ram = new unsigned char[8192];
    }

    ~c_mapper190()
    {
        delete[] ram;
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        switch ((address >> 12) & 0xE) {
            case 0x6:
                ram[address - 0x6000] = value;
                break;
            case 0x8:
                SetPrgBank16k(PRG_8000, value & 0x7);
                break;
            case 0xA:
                SetChrBank2k((address & 0x3) << 1, value);
                break;
            case 0xC:
                SetPrgBank16k(PRG_8000, 0x8 | (value & 0x7));
                break;
            default:
                break;
        }
    }

    unsigned char read_byte(unsigned short address) override
    {
        if (address >= 0x6000 && address < 0x8000) {
            return ram[address - 0x6000];
        }
        else
            return c_mapper::read_byte(address);
    }

    void reset() override
    {
        c_mapper::reset();
        set_mirroring(MIRRORING_VERTICAL);
        SetPrgBank16k(PRG_C000, 0);
        memset(ram, 0, 8192);
    }

  private:
    unsigned char *ram;
};

} //namespace nes
