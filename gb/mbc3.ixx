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
                .pak_features = PAK_FEATURES::NONE,
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
    c_mbc3();
    void write_byte(uint16_t address, uint8_t data);
    uint8_t read_byte(uint16_t address);
    void reset();

  private:
    int bank;
    int ram_enable;
    int ram_bank;
    int rom_banks;
};

} //namespace gb