module;
#include <memory>

export module nes:mapper.mapper9;
import :mapper;
import class_registry;
import std;

namespace nes
{

export class c_mapper9 : public c_mapper, register_class<nes_mapper_registry, c_mapper9>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {{
            .number = 9,
            .name = "MMC2",
            .constructor = []() { return std::make_unique<c_mapper9>(); },
        }};
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        switch (address >> 12) {
            case 0xA:
                SetPrgBank8k(PRG_8000, value);
                break;
            case 0xB:
                latch0FD = value & 0x1F;
                if (latch0 == 0xFD)
                    SetChrBank4k(CHR_0000, value & 0x1F);
                break;
            case 0xC:
                latch0FE = value & 0x1F;
                if (latch0 == 0xFE)
                    SetChrBank4k(CHR_0000, value & 0x1F);
                break;
            case 0xD:
                latch1FD = value & 0x1F;
                if (latch1 == 0xFD)
                    SetChrBank4k(CHR_1000, value & 0x1F);
                break;
            case 0xE:
                latch1FE = value & 0x1F;
                if (latch1 == 0xFE)
                    SetChrBank4k(CHR_1000, value & 0x1F);
                break;
            case 0xF:
                if (value & 0x1)
                    set_mirroring(MIRRORING_HORIZONTAL);
                else
                    set_mirroring(MIRRORING_VERTICAL);
                break;
            default:
                c_mapper::write_byte(address, value);
                break;
        }
    }

    void reset() override
    {
        SetPrgBank8k(PRG_8000, 0);
        SetPrgBank8k(PRG_A000, prgRomPageCount8k - 3);
        SetPrgBank8k(PRG_C000, prgRomPageCount8k - 2);
        SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);

        latch0FD = 0;
        latch0FE = 4;
        latch1FD = 0;
        latch1FE = 0;
        latch0 = 0xFE;
        latch1 = 0xFE;

        SetChrBank4k(CHR_0000, latch0FE);
        SetChrBank4k(CHR_1000, latch1FE);

        memset(latch_buffer, 0, sizeof(latch_buffer));
        latch_buffer_head = 1;
        latch_buffer_tail = 0;
    }

    unsigned char read_chr(unsigned short address) override
    {
        latch_buffer[latch_buffer_head++ % 2] = address;
        unsigned char temp = c_mapper::read_chr(address);
        mmc2_latch(latch_buffer[latch_buffer_tail++ % 2]);
        return temp;
    }

private:
    int latch0;
    int latch1;
    int latch0FD;
    int latch0FE;
    int latch1FD;
    int latch1FE;
    int latch_buffer[2];
    unsigned int latch_buffer_head;
    unsigned int latch_buffer_tail;

    void mmc2_latch(int address)
    {
        switch (address &= 0x1FF0) {
            case 0x0FD0:
                SetChrBank4k(CHR_0000, latch0FD);
                latch0 = 0xFD;
                break;
            case 0x0FE0:
                SetChrBank4k(CHR_0000, latch0FE);
                latch0 = 0xFE;
                break;
            case 0x1FD0:
                SetChrBank4k(CHR_1000, latch1FD);
                latch1 = 0xFD;
                break;
            case 0x1FE0:
                SetChrBank4k(CHR_1000, latch1FE);
                latch1 = 0xFE;
                break;
            default:
                break;
        }
    }
};

} //namespace nes
