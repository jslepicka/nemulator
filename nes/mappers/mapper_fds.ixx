module;
#include <fstream>
#include <Windows.h>

export module nes:mapper_fds;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{
class c_mapper_fds : public c_mapper, register_class<nes_mapper_registry, c_mapper_fds>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 0x103,
                .name = "FDS",
                .constructor = []() { return std::make_unique<c_mapper_fds>(); },
            },
        };
    }

    c_mapper_fds()
    {
        ram = std::make_unique<uint8_t[]>(40 * 1024);
        chr_ram = std::make_unique<uint8_t[]>(8 * 1024);
    }

    void reset()
    {
    }

  private:
    std::unique_ptr<uint8_t[]> ram;
    std::unique_ptr<uint8_t[]> chr_ram;
    uint8_t expansion;
    uint8_t motor_on;

    unsigned char read_byte(unsigned short address) override
    {
        if (address < 0x6000) {
            char buf[64];
            sprintf(buf, "fds read: %04x\n", address);
            OutputDebugString(buf);
            int x = 1;
            switch (address) {
                case 0x4032: //disk status
                    return 0x0;
                case 0x4033: //expansion
                    return (motor_on << 7) | (expansion & 0x7F); //battery ok
                default:
                    return 0;
            }
        }
        else {
            return ram[address - 0x6000];
        }
        return 0;
    }
    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address < 0x6000) {
            char buf[64];
            sprintf(buf, "fds write: %04x = %02x\n", address, value);
            OutputDebugString(buf);
            int x = 1;
            switch (address) {
                case 0x4025:
                    motor_on = value & 0x2 ? 0 : 1;
                    set_mirroring(value & 0x8 ? MIRRORING_HORIZONTAL : MIRRORING_VERTICAL);
                case 0x4026:
                    expansion = value;
                    break;
                default:
                    break;
            }
        }
        else {
            ram[address - 0x6000] = value;
        }
    }
    unsigned char read_chr(unsigned short address) override
    {
        return chr_ram[address];
    }
    void write_chr(unsigned short address, unsigned char value) override
    {
        chr_ram[address] = value;
    }
    int load_image() override
    {
        int x = 1;
        std::ifstream rom;
        rom.open("c:\\roms\\fds\\disksys.rom", std::ios_base::in | std::ios_base::binary);
        if (rom.is_open()) {
            rom.read((char *)ram.get() + 32768, 8192);
            rom.close();
        }
        return 1;
    }
};
} //namespace nes
