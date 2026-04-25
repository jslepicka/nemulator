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
                         .fb_height = 240,
                         .crop_top = 8,
                         .crop_bottom = 8
                     },
                 .button_map = {
                     {BUTTON_1A,      0x01},
                     {BUTTON_1B,      0x02},
                     {BUTTON_1SELECT, 0x04},
                     {BUTTON_1START,  0x08},
                     {BUTTON_1UP,     0x10},
                     {BUTTON_1RIGHT,  0x20},
                     {BUTTON_1DOWN,   0x40},
                     {BUTTON_1LEFT,   0x80}
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
            cpu->available_cycles += 390 * 3;
            cpu->execute();
            vid->do_scanline();
            cpu->available_cycles += (455 - 390) * 3;
            cpu->execute();
            
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
        joy_write = 0;
        joy = 0xFF;
        return 0;
    }

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
        int offset = 0;
        //if (file_length % 1024) {
        //    ods("%s - header detected, skipping 512 bytes\n", path_file.c_str());
        //    offset = 512;
        //    file_length -= 512;
        //}
        file.seekg(offset, std::ios_base::beg);
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
        joy = ~input;   
    }

    uint8_t read_byte(uint32_t address)
    {
        int bank = address >> 13;
        const int mask = (1 << 13) - 1;
        if (bank < 0x80) {
            uint32_t base = bank * 8192;
            uint32_t offset = address & 0x1FFFF;
            if (file_length / 8192 == 48) {
                // goofy ass mirroring
                uint32_t b = bank >> 4;
                static const int o[] = {0, 0x10, 0, 0x10, 0x20, 0x20, 0x20, 0x20};
                base = o[b] * 8192;
                address = base + offset;

            }
            else if (file_length / 8192 == 64) {
                uint32_t b = bank >> 4;
                static const int o[] = {0, 0x10, 0x20, 0x30, 0x20, 0x30, 0x20, 0x30};
                base = o[b] * 8192;
                address = base + offset;
            }
            //assert(address == base + offset);
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
                //ods("read from VCE\n");
                return vid->read_vdc(address);
            }
            else if (address < 0xC00) {
                ods("read from PSG\n");
                return 0;
            }
            else if (address < 0x1000) {
                assert(0);
            }
            else if (address < 0x1400) {
                //return 0x3F;
                uint8_t ret =
                    (1 << 7) | // cd not attached
                    (0 << 6) | // region = us
                    (1 << 5) | // unused
                    (1 << 4); //unused
                if (joy_write & 0x1) {
                    //directions
                    ret |= ((joy >> 4) & 0xF);
                }
                else {
                    //buttons
                    ret |= (joy & 0xF);
                }
                return ret;
            }
            else if (address < 0x1800) {
                return 0;
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
                //ods("write %2X to VDC address %4X\n", value, address);
                vid->write_vdc(address, value);
            }
            else if (address < 0x800) {
                //ods("write %2X to VCE address %4X\n", value, address);
                vid->write_vce(address, value);
            }
            else if (address < 0xC00) {
                //ods("write %2X to PSG address %4X\n", value, address);
            }
            else if (address < 0x1000) {
                ods("write %2X to timer address %4X\n", value, address);
            }
            else if (address < 0x1400) {
                //ods("write %2X to IO address %4X\n", value, address);
                joy_write = value;
            }
            else if (address < 0x1800) {
                ods("write %2X to interrupt controller address %4X\n", value, address);
                if (address == 0x1402) {
                    irq_controller_1402 = value;
                }
            }
            else {
                assert(0);
            }
        }
        else {
            assert(0);
        }
    }

    void write_vid(uint8_t address, uint8_t value)
    {
        vid->write_vdc(address, value);
    }

  public:
    int irq1;
    int irq2;
    uint8_t irq_controller_1402;

  private:
    std::unique_ptr<c_huc6280<c_tg16>> cpu;
    std::unique_ptr<c_vid<c_tg16>> vid;
    std::unique_ptr<uint8_t[]> rom;
    uint8_t ram[8192];
    bool loaded;
    uint8_t joy_write;
    uint8_t joy;
};
} //namespace tg16