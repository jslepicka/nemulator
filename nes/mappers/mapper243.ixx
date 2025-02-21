module;
#include "..\mirroring_types.h"
export module nes_mapper.mapper243;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

//Sachen games
//2-in-1 Cosmo Cop

class c_mapper243 : public c_mapper, register_class<nes_mapper_registry, c_mapper243>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 243,
                .constructor = []() { return std::make_unique<c_mapper243>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x4020 && address < 0x5000) {
            switch (address & 0x4101) {
                case 0x4100:
                    command = value & 0x7;
                    break;
                case 0x4101:
                    switch (command) {
                        case 2:
                            chr = (chr & 0x7) | ((value & 0x1) << 3);
                            sync_chr();
                            break;
                        case 4:
                            chr = (chr & 0xB) | ((value & 0x1) << 2);
                            sync_chr();
                            break;
                        case 5:
                            SetPrgBank32k(value & 0x7);
                            break;
                        case 6:
                            chr = (chr & 0xC) | (value & 0x3);
                            sync_chr();
                            break;
                        case 7:
                            switch ((value >> 1) & 0x3) {
                                case 0:
                                    set_mirroring(MIRRORING_HORIZONTAL);
                                    break;
                                case 1:
                                    set_mirroring(MIRRORING_VERTICAL);
                                    break;
                                case 2:
                                    set_mirroring(MIRRORING_ONESCREEN_HIGH);
                                    name_table[0] = &vram[0];
                                    break;
                                case 3:
                                    set_mirroring(MIRRORING_ONESCREEN_HIGH);
                                    break;
                            }
                            break;
                    }
                    break;
            }
        }
    }

    void reset() override
    {
        chr = 0;
        command = 0;
    }

  private:
    int chr;
    int command;
    void sync_chr()
    {
        SetChrBank8k(chr);
    }
};

} //namespace nes
