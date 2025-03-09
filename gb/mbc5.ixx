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
    c_mbc5();
    void write_byte(uint16_t address, uint8_t data);
    uint8_t read_byte(uint16_t address);
    void reset();

  private:
    int bank_lo;
    int bank_hi;
    int bank;
    int bank_fixup;
    int ram_bank;
    int ram_enable;
    int mode;

    void fixup_bank();
    int rom_banks;
};

} //namespace gb