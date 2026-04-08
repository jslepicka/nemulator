module;

export module gb:mapper.mbc5;

import :mapper;
import class_registry;

namespace gb
{

class c_mbc5 : public c_gbmapper, register_class<mapper_registry, c_mbc5>
{
  public:
    static std::vector<c_gbmapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 0x19,
                .name = "MBC5",
                .constructor = []() { return std::make_unique<c_mbc5>(); },
            },
            {
                .number = 0x1A,
                .name = "MBC5 + RAM",
                .pak_features = PAK_FEATURES::RAM,
                .constructor = []() { return std::make_unique<c_mbc5>(); },
            },
            {
                .number = 0x1B,
                .name = "MBC5 + RAM + BATTERY",
                .pak_features = PAK_FEATURES::RAM | PAK_FEATURES::BATTERY,
                .constructor = []() { return std::make_unique<c_mbc5>(); },
            },
            {
                .number = 0x1C,
                .name = "MBC5 + RUMBLE",
                .pak_features = PAK_FEATURES::RUMBLE,
                .constructor = []() { return std::make_unique<c_mbc5>(); },
            },
            {
                .number = 0x1D,
                .name = "MBC5 + RAM + RUMBLE",
                .pak_features = PAK_FEATURES::RAM | PAK_FEATURES::RUMBLE,
                .constructor = []() { return std::make_unique<c_mbc5>(); },
            },
            {
                .number = 0x1E,
                .name = "MBC5 + RAM + BATTERY + RUMBLE",
                .pak_features = PAK_FEATURES::RAM | PAK_FEATURES::BATTERY | PAK_FEATURES::RUMBLE,
                .constructor = []() { return std::make_unique<c_mbc5>(); },
            },

        };
    }

    c_mbc5() = default;

    void write_byte(uint16_t address, uint8_t data)
    {
        switch (address >> 12) {
            case 0:
            case 1:
                ram_enable = (data & 0x0F) == 0x0A;
                bank = (bank_hi << 8) | bank_lo;
                break;
            case 2:
                bank_lo = data & 0xFF;
                bank = (bank_hi << 8) | bank_lo;
                break;
            case 3:
                bank_hi = data & 0x1;
                bank = (bank_hi << 8) | bank_lo;
                break;
            case 4:
            case 5:
                ram_bank = data & (rumble ? 0xE : 0xF);
                break;
            case 0xA:
            case 0xB:
                if (ram && ram_enable)
                    ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size] = data;
                break;
            default: {
                int x = 1;
            } break;
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
                a += (bank % rom_banks) * 0x4000;
                return *(rom + a);
                break;
            case 5: //A000-BFFF ram
                if (ram && ram_enable)
                    return ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size];
                break;
        }
        return 0;
    }

    void reset()
    {
        rom_banks = (32 << *(rom + 0x148)) / 16;
        bank = 0;
        bank_fixup = 1;
        ram_enable = 0;
        mode = 0;
        ram_bank = 0;
        bank_lo = 0;
        bank_hi = 0;
        c_gbmapper::reset();
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