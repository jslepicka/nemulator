module;

export module gb:mapper.mbc3;
import :mapper;
import class_registry;

namespace gb
{

class c_mbc3 : public c_gbmapper, register_class<mapper_registry, c_mbc3>
{
  public:
    static std::vector<c_gbmapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 0x0F,
                .name = "MBC3 + TIMER + BATTERY",
                .pak_features = PAK_FEATURES::TIMER | PAK_FEATURES::BATTERY,
                .constructor = []() { return std::make_unique<c_mbc3>(); },
            },
            {
                .number = 0x10,
                .name = "MBC3 + TIMER + RAM + BATTERY",
                .pak_features = PAK_FEATURES::TIMER | PAK_FEATURES::RAM | PAK_FEATURES::BATTERY,
                .constructor = []() { return std::make_unique<c_mbc3>(); },
            },
            {
                .number = 0x11,
                .name = "MBC3",
                .constructor = []() { return std::make_unique<c_mbc3>(); },
            },
            {
                .number = 0x12,
                .name = "MBC3 + RAM",
                .pak_features = PAK_FEATURES::RAM,
                .constructor = []() { return std::make_unique<c_mbc3>(); },
            },
            {
                .number = 0x13,
                .name = "MBC3 + RAM + BATTERY",
                .pak_features = PAK_FEATURES::RAM | PAK_FEATURES::BATTERY,
                .constructor = []() { return std::make_unique<c_mbc3>(); },
            },
        };
    }

    c_mbc3() = default;

    void write_byte(uint16_t address, uint8_t data)
    {
        int x;
        switch (address >> 13) {
            case 0: //0000-1FFFF ram enable
                ram_enable = data;
                break;
            case 1: //2000-3FFF rom bank
                bank = data == 0 ? 1 : data & 0x7F;
                bank %= rom_banks;
                break;
            case 2: //4000-5FFF ram bank or rtc register select
                if (data < 8)
                    ram_bank = data;
                break;
            case 3: //6000-7FFF latch clock data
                x = 1;
                break;
            case 5: //A000-BFFF RAM
                if (ram_enable == 0x0A)
                    ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size] = data;
                break;
            default:
                int x = 1;
                break;
        }
        c_gbmapper::write_byte(address, data);
    }

    uint8_t read_byte(uint16_t address)
    {
        switch (address >> 13) {
            case 0: //0000-3FFF rom 0
            case 1:
                return c_gbmapper::read_byte(address);
                break;
            case 2: //4000-7FFF rom bank
            case 3:
                return *(rom + (address & 0x3FFF) + bank * 0x4000);
                break;
            case 5: //A000-BFFF ram
                if (ram_enable == 0x0A)
                    return ram[((address & 0x1FFF) + ram_bank * 0x2000) % ram_size];
                break;
        }
        return 0;
    }

    void reset()
    {
        ram_bank = 0;
        bank = 1;
        ram_enable = 0;
        rom_banks = (32 << *(rom + 0x148)) / 16;
        c_gbmapper::reset();
    }

  private:
    int bank;
    int ram_enable;
    int ram_bank;
    int rom_banks;
};

} //namespace gb