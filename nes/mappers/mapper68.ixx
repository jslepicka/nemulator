module;
#include "..\mirroring_types.h"
export module nes_mapper.mapper68;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

//After Burner

class c_mapper68 : public c_mapper, register_class<nes_mapper_registry, c_mapper68>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 68,
                .constructor = []() { return std::make_unique<c_mapper68>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            switch (address & 0xF000) {
                case 0x8000:
                    SetChrBank2k(CHR_0000, value);
                    break;
                case 0x9000:
                    SetChrBank2k(CHR_0800, value);
                    break;
                case 0xA000:
                    SetChrBank2k(CHR_1000, value);
                    break;
                case 0xB000:
                    SetChrBank2k(CHR_1800, value);
                    break;
                case 0xC000:
                    reg_c = value & 0x7F;
                    break;
                case 0xD000:
                    reg_d = value & 0x7F;
                    break;
                case 0xE000:
                    mirroring_mode = value & 0x01;
                    nt_mode = value & 0x10;

                    if (nt_mode) {
                        if (mirroring_mode) {
                            name_table[0] = name_table[1] = pChrRom + ((reg_c | 0x80) * 0x400);
                            name_table[2] = name_table[3] = pChrRom + ((reg_d | 0x80) * 0x400);
                        }
                        else {
                            name_table[0] = name_table[2] = pChrRom + ((reg_c | 0x80) * 0x400);
                            name_table[1] = name_table[3] = pChrRom + ((reg_d | 0x80) * 0x400);
                        }
                    }
                    else {

                        if (mirroring_mode)
                            set_mirroring(MIRRORING_HORIZONTAL);
                        else
                            set_mirroring(MIRRORING_VERTICAL);
                    }
                    break;
                case 0xF000:
                    SetPrgBank16k(PRG_8000, value);
                    break;
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

    void reset() override
    {
        mirroring_mode = 0;
        nt_mode = 0;
        reg_c = 0;
        reg_d = 0;
        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
    }

    void ppu_write(unsigned short address, unsigned char value) override
    {
        if (nt_mode && (address & 0x3FFF) >= 0x2000 && (address & 0x3FFF) < 0x3F00)
            return;
        else
            c_mapper::ppu_write(address, value);
    }

  private:
    int reg_c;
    int reg_d;
    int mirroring_mode;
    int nt_mode;
};

} //namespace nes
