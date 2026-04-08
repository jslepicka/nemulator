module;

export module gb:mapper;
import nemulator.std;

import class_registry;

namespace gb
{

class c_gbmapper
{
  public:
    c_gbmapper()
    {
        ram = nullptr;
        rumble = false;
    }

    virtual ~c_gbmapper() = default;
    
    virtual void write_byte(uint16_t address, uint8_t data)
    {
    }

    virtual uint8_t read_byte(uint16_t address)
    {
        if (address < 0x8000) {
            return *(rom + address);
        }
        else {
            return 0;
        }
    }

    virtual void reset()
    {
    }
    
    virtual void config_ram(int ram_size)
    {
        this->ram_size = ram_size;
        ram = std::make_unique<uint8_t[]>(ram_size);
    }
    

    struct s_mapper_info
    {
        int number;
        std::string name;
        int pak_features = 0;
        std::function<std::unique_ptr<c_gbmapper>()> constructor;
    };

  public:
    uint8_t *rom;
    std::unique_ptr<uint8_t[]> ram;
    bool rumble;

  protected:
    uint32_t ram_size;
};

enum PAK_FEATURES
{
    RAM = 1 << 0,
    BATTERY = 1 << 1,
    RUMBLE = 1 << 2,
    TIMER = 1 << 3,
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
                .name = "No mapper",
                .constructor = []() { return std::make_unique<c_mapper_none>(); },
            },
        };
    }
};

} //namespace gb