module;
#include <cstdint>
export module gb:mapper.mbc1;
import :mapper;
import class_registry;

namespace gb
{

class c_mbc1 : public c_gbmapper, register_class<mapper_registry, c_mbc1>
{
  public:
    static std::vector<c_gbmapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 1,
                .name = "MBC1",
                .constructor = []() { return std::make_unique<c_mbc1>(); },
            },
            {
                .number = 2,
                .name = "MBC1 + RAM",
                .pak_features = PAK_FEATURES::RAM,
                .constructor = []() { return std::make_unique<c_mbc1>(); },
            },
            {
                .number = 3,
                .name = "MBC1 + RAM + BATTERY",
                .pak_features = PAK_FEATURES::RAM | PAK_FEATURES::BATTERY,
                .constructor = []() { return std::make_unique<c_mbc1>(); },
            },
        };
    }

    c_mbc1() = default;

    void write_byte(uint16_t address, uint8_t data)
    {
        switch (address >> 13) {
            case 0: //0000-1FFF ram enable
                ram_enable = data;
                break;
            case 1: //2000-3FFF rom bank
                bank_lo = (data & 0x1F);
                bank = (bank & 0x60) | (data & 0x1F);
                fixup_bank();
                break;
            case 2: //4000-5FFF ram bank or upper bits of rom bank number
                bank_hi = data & 0x3;
                if (mode == 0) {
                    //rom
                    bank = (bank & 0x1F) | ((data & 0x3) << 5);
                    fixup_bank();
                }
                else if (mode == 1) {
                    ram_bank = data & 0x3;
                }
                break;
            case 3: //6000-7FFF rom/ram mode select
                mode = data & 1;
                break;
            case 5: //A000-BFFF RAM
                if (ram && (ram_enable & 0xF) == 0xA)
                    ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size] = data;
                break;
            default:
                int x = 1;
                break;
        }
    }

    uint8_t read_byte(uint16_t address)
    {
        int a;
        switch (address >> 13) {
            case 0: //000-3FFF rom 0
            case 1:
                return c_gbmapper::read_byte(address);
                break;
            case 2: //4000-7FFF rom bank
            case 3:
                a = (address & 0x3FFF);
                a += bank_fixup * 0x4000;
                return *(rom + a);
                break;
            case 5: //A000-BFFF ram
                if (ram && (ram_enable & 0xF) == 0xA)
                    return ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size];
                break;
        }
        return 0;
    }

    void reset()
    {
        bank = 0;
        bank_fixup = 1;
        ram_enable = 0;
        mode = 0;
        ram_bank = 0;
        rom_banks = (32 << *(rom + 0x148)) / 16;
        c_gbmapper::reset();
    }

  private:
    void fixup_bank()
    {
        bank_fixup = bank;
        if ((bank & 0x1F) == 0 && !(bank & 0x10)) {
            bank_fixup |= 1;
        }
        bank_fixup %= rom_banks;
    }

  private:
    int bank_lo;
    int bank_hi;
    int bank;
    int bank_fixup;
    int ram_bank;
    int ram_enable;
    int mode;
    int rom_banks;
};

} //namespace gb