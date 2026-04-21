module;
#include <cassert>
export module tg16;
import nemulator.std;
import nemulator.buttons;
import crc32;
import system;
import class_registry;

import :huc6280;
import :vid;

namespace tg16
{
export class c_tg16 : public c_system, register_class<system_registry, c_tg16>
{
  public:
    static std::vector<s_system_info> get_registry_info()
    {
        return {{.name = "NEC TurboGrafx-16",
                 .identifier = "tg16",
                 .extension = "pce",
                 .display_info =
                     {
                         .fb_width = 256,
                         .fb_height = 224,
                     },
                 .constructor = []() { return std::make_unique<c_tg16>(); }}};
    }

    c_tg16()
    {
        cpu = std::make_unique<c_huc6280<c_tg16>>(*this);
        vid = std::make_unique<c_vid<c_tg16>>(*this);
        loaded = false;
    }

    ~c_tg16()
    {
    }

    int emulate_frame()
    {
        for (int i = 0; i < 263; i++) {
            cpu->available_cycles += 455 * 3;
            cpu->execute();
            vid->do_scanline();
        }
        return 0;
    }

    int reset()
    {
        cpu->reset();
        vid->reset();
        std::memset(ram, 0, sizeof(ram));
        irq1 = 0;
        irq2 = 0;
        return 0;
    }

    int fb[256 * 240];
    int *get_video()
    {
        return (int*)vid->fb;
    }

    int get_sound_buf(const float **buf)
    {
        buf = nullptr;
        return 0;
    }

    void set_audio_freq(double freq)
    {
    }
    int file_length;
    int load()
    {
        std::ifstream file;
        file.open(path_file, std::ios_base::in | std::ios_base::binary);
        if (file.fail())
            return 0;
        file.seekg(0, std::ios_base::end);
        file_length = (int)file.tellg();
        file.seekg(0, std::ios_base::beg);
        rom = std::make_unique_for_overwrite<uint8_t[]>(file_length);
        file.read((char *)rom.get(), file_length);
        file.close();
        crc32 = get_crc32(rom.get(), file_length);
        loaded = true;
        reset();
        return 1;
    }

    int is_loaded()
    {
        return loaded;
    }

    void enable_mixer()
    {
    }

    void disable_mixer()
    {
    }

    void set_input(int input)
    {
    }

    uint8_t read_byte(uint32_t address)
    {
        int bank = address >> 13;
        const int mask = (1 << 13) - 1;
        if (bank < 0x80) {
            uint32_t base = 0;
            uint32_t offset = address & 0x1FFFF;
            if (file_length / 8192 == 48) {
                // goofy ass mirroring
                uint32_t b = bank >> 4;
                static const int o[] = {0, 0x10, 0, 0x10, 0x20, 0x20, 0x20, 0x20};
                base = o[b] * 8192;

            }
            //assert(address == base + offset);
            address = base + offset;
            uint32_t k = address & mask;
            uint32_t j = address & (file_length - 1);
            assert(address <= (uint32_t)file_length);
            return rom[address];
        }
        else if (bank == 0xF8) {
            // ram
            address &= mask;
            return ram[address];
        }
        else if (bank == 0xFF) {
            address &= mask;
            // hardware registers
            // 0000-03FF - VDC (4 registers)
            // 0400-07FF - VCE (8 registers)
            // 0800-0BFF - PSG
            // 0C00-0FFF - Timer (2 registers)
            // 1000-13FF - I/O (1 register)
            // 1400-17FF - Interrupt controller (4 registers)
            // 1800-1FFF - unmapped
            if (address < 0x400) {
                return vid->read_vdc(address);
            }
            else if (address < 0x800) {
                assert(0);
            }
            else if (address < 0xC00) {
                assert(0);
            }
            else if (address < 0x1000) {
                assert(0);
            }
            else if (address < 0x1400) {
                return 0x3F;
                assert(0);
            }
            else if (address < 0x1800) {
                assert(0);
            }
            else {
                assert(0);
            }

            
        }
        else {
            assert(0);
        }
        return 0;
    }

    void write_byte(uint32_t address, uint8_t value)
    {
        int bank = address >> 13;
        const int mask = (1 << 13) - 1;
        if (bank < 0x80) {
            assert(0);
        }
        else if (bank == 0xF8) {
            address &= mask;
            ram[address] = value;
        }
        else if (bank == 0xFF) {
            address &= mask;
            // hardware registers
            // 0000-03FF - VDC (4 registers)
            // 0400-07FF - VCE (8 registers)
            // 0800-0BFF - PSG
            // 0C00-0FFF - Timer (2 registers)
            // 1000-13FF - I/O (1 register)
            // 1400-17FF - Interrupt controller (4 registers)
            // 1800-1FFF - unmapped
            if (address < 0x400) {
                //std::printf("write %2X to VDC address %4X\n", value, address);
                vid->write_vdc(address, value);
            }
            else if (address < 0x800) {
                //std::printf("write %2X to VCE address %4X\n", value, address);
                vid->write_vce(address, value);
            }
            else if (address < 0xC00) {
                //std::printf("write %2X to PSG address %4X\n", value, address);
            }
            else if (address < 0x1000) {
                std::printf("write %2X to timer address %4X\n", value, address);
            }
            else if (address < 0x1400) {
                //std::printf("write %2X to IO address %4X\n", value, address);
            }
            else if (address < 0x1800) {
                std::printf("write %2X to interrupt controller address %4X\n", value, address);
            }
            else {
                assert(0);
            }
        }
        else {
            assert(0);
        }
    }

  public:
    int irq1;
    int irq2;

  private:
    std::unique_ptr<c_huc6280<c_tg16>> cpu;
    std::unique_ptr<c_vid<c_tg16>> vid;
    std::unique_ptr<uint8_t[]> rom;
    uint8_t ram[8192];
    bool loaded;

};
} //namespace tg16