module;

export module gb:mapper;
import std.compat;
import class_registry;

namespace gb
{

class c_gbmapper
{
  public:
    c_gbmapper();
    virtual ~c_gbmapper();
    virtual void write_byte(uint16_t address, uint8_t data);
    virtual uint8_t read_byte(uint16_t address);
    virtual void reset();
    virtual void config_ram(int ram_size);
    bool rumble;
    uint8_t *rom;
    std::unique_ptr<uint8_t[]> ram;

    struct s_mapper_info
    {
        int number;
        std::string name;
        int pak_features;
        std::function<std::unique_ptr<c_gbmapper>()> constructor;
    };

  protected:
    uint32_t ram_size;
};

enum PAK_FEATURES
{
    NONE = 1 << 0,
    RAM = 1 << 1,
    BATTERY = 1 << 2,
    RUMBLE = 1 << 3,
    TIMER = 1 << 4,
};

export class mapper_registry : public c_class_registry<std::map<int, c_gbmapper::s_mapper_info>>
{
  public:
    static void _register(std::vector<c_gbmapper::s_mapper_info> mapper_info)
    {
        for (auto &mi : mapper_info) {
            if (mi.name == "") {
                mi.name = "Mapper " + std::to_string(mi.number);
            }
            get_registry()[mi.number] = mi;
        }
    }
};

class c_mapper_none : public c_gbmapper, register_class<mapper_registry, c_mapper_none>
{
  public:
    static std::vector<c_gbmapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 0,
                .name = "ROM Only",
                .pak_features = PAK_FEATURES::NONE,
                .constructor = []() { return std::make_unique<c_mapper_none>(); },
            },
        };
    }
};

} //namespace gb