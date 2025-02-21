module;
#include "..\mirroring_types.h"
#include <memory>
export module nes_mapper.vrc4;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

class c_mapper_vrc4 : public c_mapper, register_class<nes_mapper_registry, c_mapper_vrc4>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 21,
                .name = "VRC4",
                .constructor = []() { return std::make_unique<c_mapper_vrc4>(1); },
            },
            {
                .number = 22,
                .name = "VRC4",
                .constructor = []() { return std::make_unique<c_mapper_vrc4>(3); },
            },
            {
                .number = 23,
                .name = "VRC4",
                .constructor = []() { return std::make_unique<c_mapper_vrc4>(); },
            },
            {
                .number = 25,
                .name = "VRC4",
                .constructor = []() { return std::make_unique<c_mapper_vrc4>(2); },
            },
        };
    }

    c_mapper_vrc4(int submapper = 0)
    {
        this->submapper = submapper;
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        int swapped = 0;
        switch (address >> 12) {
            case 0x8:
                reg_8 = value & (submapper == 3 ? 0xF : 0x1F);
                sync();
                break;
            case 0x9:
                switch (get_bits(address)) {
                    case 0:
                    case 1:
                        switch (value & 0x03) {
                            case 0:
                                set_mirroring(MIRRORING_VERTICAL);
                                break;
                            case 1:
                                set_mirroring(MIRRORING_HORIZONTAL);
                                break;
                            case 2:
                                set_mirroring(MIRRORING_ONESCREEN_LOW);
                                break;
                            case 3:
                                set_mirroring(MIRRORING_ONESCREEN_HIGH);
                                break;
                        }
                        break;
                    case 2:
                    case 3:
                        prg_mode = value & 0x02;
                        sync();
                        break;
                    default:
                        break;
                }
                break;
            case 0xA:
                SetPrgBank8k(PRG_A000, value & (submapper == 3 ? 0xF : 0x1F));
                break;
            case 0xB:
            case 0xC:
            case 0xD:
            case 0xE: {
                int bits = get_bits(address);
                int chr_bank = (((address >> 12) - 0xB) * 2) | ((bits >> 1) & 0x01);
                chr[chr_bank] =
                    (chr[chr_bank] & (0xF0 >> ((bits & 0x01) * 4))) | ((value & 0x0F) << ((bits & 0x01) * 4));
                int value = chr[chr_bank];
                if (submapper == 3)
                    value >>= 1;
                SetChrBank1k(chr_bank, value);
            } break;
            case 0xF:
                if (submapper == 3)
                    return;
                switch (get_bits(address)) {
                    case 0:
                        irq_reload = (irq_reload & 0xF0) | (value & 0x0F);
                        break;
                    case 1:
                        irq_reload = (irq_reload & 0x0F) | ((value & 0x0F) << 4);
                        break;
                    case 2:
                        irq_control = value & 0x7;
                        if (irq_control & 0x02) {
                            irq_scaler = 0;
                            irq_counter = irq_reload;
                        }
                        clear_irq();
                        break;
                    case 3:
                        clear_irq();
                        if (irq_control & 0x01)
                            irq_control |= 0x02;
                        else
                            irq_control &= ~0x02;
                        break;
                }
                break;
            default:
                c_mapper::write_byte(address, value);
                break;
        }
    }

    void reset() override
    {
        memset(chr, 0, sizeof(chr));

        SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
        irq_counter = 0;
        irq_scaler = 0;
        irq_reload = 0;
        irq_control = 0;
        reg_a = 0;
        reg_8 = 0;
        prg_mode = 0;

        if (submapper == 2)
            swap_bits = 1;
    }

    void clock(int cycles) override
    {
        if ((irq_control & 0x02) && ((irq_control & 0x04) || ((irq_scaler += cycles) >= 341))) {
            if (!(irq_control & 0x04)) {
                irq_scaler -= 341;
            }
            if (irq_counter == 0xFF) {
                irq_counter = irq_reload;
                execute_irq();
            }
            else
                irq_counter++;
        }
    }


  private:
    int swap_bits;
    int chr[8];
    int irq_control;
    int irq_reload;
    int irq_counter;
    int irq_scaler;
    int reg_8;
    int reg_a;
    int prg_mode;

    int get_bits(int address)
    {
        switch (submapper) {
            case 0:
                address |= (address >> 2) | (address >> 4) | (address >> 6);
                address &= 0x03;
                break;
        //if (swap_bits)
        //    address = ((address & 0x01) << 1) | ((address >> 1) & 0x01);
            case 1:
                address = (address >> 1) | (address >> 6);
                address &= 0x03;
                break;
            case 2:
                address |= (address >> 2) | (address >> 4) | (address >> 6);
                address &= 0x03;
                address = ((address & 0x01) << 1) | ((address >> 1) & 0x01);
                break;
            case 3:
                switch (address & 0x3) {
                    case 1:
                        address += 1;
                        break;
                    case 2:
                        address -= 1;
                        break;
                    default:
                        break;
                }

                address &= 0x3;
                break;
        }
        return address;
    }

    void sync()
    {
        SetPrgBank8k(prg_mode, reg_8);
        SetPrgBank8k(2 ^ prg_mode, prgRomPageCount8k - 2);
    }
};

} //namespace nes
