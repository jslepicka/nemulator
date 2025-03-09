export module nes:mapper.mapper77;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

//Napoleon Senki (J) [!].nes

class c_mapper77 : public c_mapper, register_class<nes_mapper_registry, c_mapper77>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 77,
                .constructor = []() { return std::make_unique<c_mapper77>(); },
            },
        };
    }

    c_mapper77()
    {
        chr_ram = new unsigned char[8192];
    }

    ~c_mapper77()
    {
        delete[] chr_ram;
        chrRam = false;
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            SetPrgBank32k(value & 0xF);
            SetChrBank2k(CHR_0000, value >> 4);
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        for (int i = 0; i < 6; i++) {
            chrBank[2 + i] = chr_ram + 1024 * i;
        }
        chrRam = true;
    }

  private:
    unsigned char *chr_ram;
};

} //namespace nes
