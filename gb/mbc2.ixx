module;

export module gb:mapper.mbc2;
import :mapper;
import class_registry;

namespace gb
{

class c_mbc2 : public c_gbmapper, register_class<mapper_registry, c_mbc2>
{
  public:
    static std::vector<c_gbmapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 5,
                .name = "MBC2",
                .pak_features = PAK_FEATURES::NONE,
                .constructor = []() { return std::make_unique<c_mbc2>(); },
            },
            {
                .number = 6,
                .name = "MBC2 + BATTERY",
                .pak_features = PAK_FEATURES::BATTERY,
                .constructor = []() { return std::make_unique<c_mbc2>(); },
            },
        };
    }
    c_mbc2();
    void write_byte(uint16_t address, uint8_t data);
    uint8_t read_byte(uint16_t address);
    void reset();

  private:
    int bank;
    int ram_bank;
    int ram_enable;
    int mode;
    int rom_banks;
};

} //namespace gb