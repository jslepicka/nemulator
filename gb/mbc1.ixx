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
    c_mbc1();
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