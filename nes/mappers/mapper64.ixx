module;
#include "..\mapper.h"
export module nes_mapper.mapper64;

namespace nes
{

class c_mapper64 : public c_mapper, register_class<nes_mapper_registry, c_mapper64>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 64,
                .name = "RAMBO-1",
                .constructor = []() { return std::make_unique<c_mapper64>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            switch (address & 0xE001) {
                case 0x8000:
                    reg_8000 = value;
                    break;
                case 0x8001:
                    reg_8001 = value;
                    switch (reg_8000 & 0xF) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                            chr[reg_8000 & 0xF] = value;
                            break;
                        case 6:
                        case 7:
                            prg[(reg_8000 & 0xF) - 6] = value;
                            break;
                        case 8:
                        case 9:
                            chr[(reg_8000 & 0xF) - 2] = value;
                            break;
                        case 0xF:
                            prg[2] = value;
                            break;
                    }
                    break;
                case 0xa000:
                    reg_a000 = value;
                    break;
                case 0xc000:
                    reg_c000 = value;
                    //irq_counter = value + (cycles_since_irq < 8 ? 1 : 2);
                    //irq_counter_reload = 1;
                    last_c000 = reg_c000;
                    break;
                case 0xc001:
                    //latched_value = reg_c000;
                    reg_c001 = value;
                    irq_counter_reload = 1;
                    cpu_divider = 0;
                    break;
                case 0xe000:
                    reg_e000 = value;
                    if (irq_asserted) {
                        clear_irq();
                        irq_asserted = 0;
                    }
                    irq_enabled = 0;
                    break;
                case 0xe001:
                    reg_e001 = value;
                    irq_enabled = 1;
                    break;
                default:
                    break;
            }
            sync();
        }
    }

    unsigned char ppu_read(unsigned short address) override
    {
        if (!in_sprite_eval)
            check_a12(address);
        return c_mapper::ppu_read(address);
    }

    void ppu_write(unsigned short address, unsigned char value) override
    {
        if (!in_sprite_eval)
            check_a12(address);
        c_mapper::ppu_write(address, value);
    }

    void reset() override
    {
        reg_8000 = reg_8001 = reg_a000 = reg_c000 = reg_c001 = reg_e000 = reg_e001 = 0;

        last_c000 = 0;

        SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);

        prg[0] = 0;
        prg[1] = 1;
        prg[2] = 2;
        chr[0] = ~0;
        chr[1] = ~0;
        chr[2] = ~0;
        chr[3] = ~0;
        chr[4] = ~0;
        chr[5] = ~0;
        chr[6] = ~0;
        chr[7] = ~0;
        sync();

        current_address = 0;
        low_count = 0;
        irq_counter = 0;
        irq_counter_reload = 0;
        irq_enabled = 0;
        irq_asserted = 0;
        irq_delay = 0;
        clock_scale = 0;
        ticks = 0;
        reload_cpu_counter = 0;
        cpu_divider = 0;
        irq_adjust = 0;

        cycles_since_irq = 0;
    }

    void clock(int cycles) override
    {
        if (!(current_address & 0x1000)) {
            low_count -= cycles;
            if (low_count < 0)
                low_count = 0;
        }
        else
            low_count = 15;
        cycles_since_irq += cycles;
        if (irq_delay > 0) {
            irq_delay -= cycles;
            if (irq_delay <= 0) {
                execute_irq();
                irq_asserted = 1;
                cycles_since_irq = 0;
            }
        }

        ticks += cycles;
        while (ticks > 2) {
            ticks -= 3;
            if (++cpu_divider == 4) {
                if ((reg_c001 & 1) && irq_enabled) {
                    clock_irq_counter();
                //if (irq_delay > 0)
                //    irq_delay = 24;
                }
                cpu_divider = 0;
            }
        }
    }

  protected:
    int chr[8];
    int prg[3];
    int reg_8000;
    int reg_8001;
    int reg_a000;
    int reg_c000;
    int reg_c001;
    int reg_e000;
    int reg_e001;
    int current_address;
    int low_count;
    unsigned char irq_counter;
    int irq_counter_reload;
    int irq_enabled;
    int irq_asserted;
    int irq_delay;
    int clock_scale;
    int ticks;
    int reload_cpu_counter;
    int cpu_divider;
    int irq_adjust;
    int next_reload;
    int a12_divider;
    int last_c000;
    int cycles_since_irq;
    int latched_value;

    void clock_irq_counter()
    {
        //if (!irq_enabled)
        //    return;
        //int x = ppu->current_cycle;
        //int y = ppu->current_scanline;
        if (irq_counter_reload) {
            irq_counter = reg_c000 | 1;
            irq_counter_reload = 0;
        }
        else if (irq_counter == 0) {
            irq_counter = reg_c000;
        }
        else {
            irq_counter--;
            if (irq_counter == 0 && irq_enabled) {
                if (!irq_asserted) {
                //int y = ppu->current_scanline;
                //int x = ppu->current_cycle;
                    fire_irq();
                }
            }
        }
    }

    void sync()
    {
        if (reg_a000 & 1) {
            set_mirroring(MIRRORING_HORIZONTAL);
        }
        else {
            set_mirroring(MIRRORING_VERTICAL);
        }

        if (reg_8000 & 0x40) //prg mode 1
        {
            SetPrgBank8k(PRG_8000, prg[2]);
            SetPrgBank8k(PRG_A000, prg[0]);
            SetPrgBank8k(PRG_C000, prg[1]);
        }
        else  //prg mode 0
        {
            SetPrgBank8k(PRG_8000, prg[0]);
            SetPrgBank8k(PRG_A000, prg[1]);
            SetPrgBank8k(PRG_C000, prg[2]);
        }

        if (reg_8000 & 0x20) //k=1
        {
            if (reg_8000 & 0x80) //c=1
            {
                SetChrBank1k(CHR_1000, chr[0]);
                SetChrBank1k(CHR_1400, chr[6]);
                SetChrBank1k(CHR_1800, chr[1]);
                SetChrBank1k(CHR_1C00, chr[7]);
                for (int i = 0; i < 4; i++)
                    SetChrBank1k(CHR_0000 + i, chr[2 + i]);
            }
            else //c=0
            {
                SetChrBank1k(CHR_0000, chr[0]);//0
                SetChrBank1k(CHR_0400, chr[6]);//6
                SetChrBank1k(CHR_0800, chr[1]);//1
                SetChrBank1k(CHR_0C00, chr[7]);//7
                for (int i = 0; i < 4; i++)
                    SetChrBank1k(CHR_1000 + i, chr[2 + i]);
            }
        }
        else //k=0
        {
            if (reg_8000 & 0x80) //c=1
            {
                SetChrBank2k(CHR_1000, chr[0] >> 1);
                SetChrBank2k(CHR_1800, chr[1] >> 1);
                for (int i = 0; i < 4; i++)
                    SetChrBank1k(CHR_0000 + i, chr[2 + i]);
            }
            else //c=0
            {
                SetChrBank2k(CHR_0000, chr[0] >> 1);
                SetChrBank2k(CHR_0800, chr[1] >> 1);
                for (int i = 0; i < 4; i++)
                    SetChrBank1k(CHR_1000 + i, chr[2 + i]);
            }
        }
    }

    void check_a12(int address)
    {
        current_address = address;
        //    int x = ppu->current_cycle;
        //int y = ppu->current_scanline;
        if (!(reg_c001 & 1) && (address & 0x1000) && low_count == 0)
            clock_irq_counter();
    }

    virtual void fire_irq()
    {
        irq_delay = 6;
    }
};

} //namespace nes
