module;

export module nes:mapper.mapper1;
import :mapper;
import class_registry;
import std;

namespace nes
{

export class c_mapper1 : public c_mapper, register_class<nes_mapper_registry, c_mapper1>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {{
            .number = 1,
            .name = "MMC1",
            .constructor = []() { return std::make_unique<c_mapper1>(); },
        }};
    }
    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000 && !ignore_writes) {
            ignore_writes = 1;
            if (value & 0x80) {
                val = 0;
                count = 0;
                regs[0] |= 0x0C;
            //return;
            }
            else {

                val |= ((value & 0x01) << count++);
                if (count == 5) {
                    regs[(address >> 13) - 4] = val;
                    count = 0;
                    val = 0;
                    Sync();
                }
            }
        }
        else
            c_mapper::write_byte(address, value);
    }
    void reset() override
    {
        regs[0] = 0x0C;
        regs[1] = 0;
        regs[2] = 0;
        regs[3] = 0;
        val = 0;
        count = 0;
        ignore_writes = 0;
        cycle_count = 1;
        Sync();
    }
    void clock(int cycles)
    {
        ignore_writes = 0;
    }

  protected:
    int ignore_writes;
    int cycle_count;
    void Sync()
    {
        sync_prg();
        sync_chr();
        sync_mirror();
    }
    virtual void sync_prg()
    {
        //reg[0] chr mode, prg size, slot select, mirroring
        //reg[1] chr reg 0
        //reg[2] chr reg 1
        //reg[3] prg reg, wram

        if (regs[0] & 0x08) //16k mode
        {
            if (regs[0] & 0x04) //$8000 swappable, $C000 fixed to page $0F
            {
                if (submapper == 1) {
                    SetPrgBank16k(PRG_8000, (regs[3] & 0x0F) | (regs[1] & 0x10));
                    SetPrgBank16k(PRG_C000, 0x0F | (regs[1] & 0x10));
                }
                else {
                    SetPrgBank16k(PRG_8000, regs[3] & 0x0F);
                    SetPrgBank16k(PRG_C000, prgRomPageCount16k - 1);
                }

            //SUROM
            //SetPrgBank16k(PRG_8000, (regs[3] & 0x0F) | (regs[1] & 0x10));
            //SetPrgBank16k(PRG_C000, 0x0F | (regs[1] & 0x10));
            }
            else //$C000 swappable, $8000 fixed to page $00
            {
                if (submapper == 1) {
                    SetPrgBank16k(PRG_C000, (regs[3] & 0x0F) | (regs[1] & 0x10));
                    SetPrgBank16k(PRG_8000, 0 | (regs[1] & 0x10));
                }
                else {
                    SetPrgBank16k(PRG_C000, regs[3] & 0x0F);
                    SetPrgBank16k(PRG_8000, 0);
                }

            //SUROM
            //SetPrgBank16k(PRG_C000, (regs[3] & 0x0F) | (regs[1] & 0x10));
            //SetPrgBank16k(PRG_8000, 0 | (regs[1] & 0x10));
            }
        }
        else //32k mode
        {
            SetPrgBank32k((regs[3] & 0x0F) >> 1);
        }
    }
    virtual void sync_chr()
    {
        if (regs[0] & 0x10) //4k chr mode
        {
            SetChrBank4k(CHR_0000, regs[1] & 0x1F);
            SetChrBank4k(CHR_1000, regs[2] & 0x1F);
        }
        else //8k chr mode
        {
            SetChrBank8k((regs[1] & 0x1F) >> 1);
        }
    }
    void sync_mirror()
    {
        switch (regs[0] & 0x03) {
            case 0x00:
                set_mirroring(MIRRORING_ONESCREEN_LOW);
                break;
            case 0x01:
                set_mirroring(MIRRORING_ONESCREEN_HIGH);
                break;
            case 0x02:
                set_mirroring(MIRRORING_VERTICAL);
                break;
            case 0x03:
                set_mirroring(MIRRORING_HORIZONTAL);
                break;
        }
    }
    unsigned char regs[4];

    int count;
    unsigned char val;
    struct c_mapperConfig
    {
        unsigned char mirroring : 1;
        unsigned char onescreen : 1;
        unsigned char prgarea : 1;
        unsigned char prgsize : 1;
        unsigned char vrom : 1;
        unsigned char unused : 3;
    } *config;
};

} //namespace nes
