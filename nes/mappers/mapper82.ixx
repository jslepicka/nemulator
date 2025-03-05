module;

export module nes_mapper.mapper82;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

//SD Keiji - Blader

class c_mapper82 : public c_mapper, register_class<nes_mapper_registry, c_mapper82>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 82,
                .constructor = []() { return std::make_unique<c_mapper82>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if ((address & 0x7EF0) == 0x7EF0) {
            switch (address & 0xF) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    chr[address & 0xF] = value;
                    sync();
                    break;
                case 6:
                    chr_mode = value & 0x2;
                    if (value & 0x1)
                        set_mirroring(MIRRORING_VERTICAL);
                    else
                        set_mirroring(MIRRORING_HORIZONTAL);
                    sync();
                    break;
                case 0xA:
                    SetPrgBank8k(PRG_8000, value >> 2);
                    break;
                case 0xB:
                    SetPrgBank8k(PRG_A000, value >> 2);
                    break;
                case 0xC:
                    SetPrgBank8k(PRG_C000, value >> 2);
                    break;
            }
        }
    }

    void reset() override
    {
        chr_mode = 0;
        chr[0] = 0;
        chr[1] = 2;
        chr[2] = 4;
        chr[3] = 5;
        chr[4] = 6;
        chr[5] = 7;
        SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
    }

  private:
    int chr_mode;
    int chr[6];
    void sync()
    {
        int _xor = chr_mode ? 4 : 0;
        SetChrBank2k(CHR_0000 ^ _xor, chr[0] >> 1);
        SetChrBank2k(CHR_0800 ^ _xor, chr[1] >> 1);
        SetChrBank1k(CHR_1000 ^ _xor, chr[2]);
        SetChrBank1k(CHR_1400 ^ _xor, chr[3]);
        SetChrBank1k(CHR_1800 ^ _xor, chr[4]);
        SetChrBank1k(CHR_1C00 ^ _xor, chr[5]);
    }
};

} //namespace nes
