module;

export module nes_mapper.mapper103;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

//Doki Doki Panic FDS conversion

class c_mapper103 : public c_mapper, register_class<nes_mapper_registry, c_mapper103>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 103,
                .name = "BTL 2708",
                .constructor = []() { return std::make_unique<c_mapper103>(); },
            },
        };
    }

    c_mapper103()
    {
        ram = new unsigned char[0x4000];
    }

    ~c_mapper103()
    {
        ram = new unsigned char[0x4000];
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x6000) {
            switch (address >> 12) {
                case 0x6:
                case 0x7:
                    *(ram + (address & 0x1FFF)) = value;
                    break;
                case 0x8:
                    prg_6000 = pPrgRom + (((value & 0xF) % prgRomPageCount8k) * 0x2000);
                    break;
                case 0xB:
                case 0xC:
                case 0xD:
                    if (address >= 0xB800 && address < 0xD800)
                        *(ram + 0x2000 + ((address - 0xB800) & 0x1FFF)) = value;
                    break;
                case 0xE:
                    if (value & 0x8)
                        set_mirroring(MIRRORING_HORIZONTAL);
                    else
                        set_mirroring(MIRRORING_VERTICAL);
                    break;
                case 0xF:
                    rom_mode = value & 0x10;
                    break;
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

    unsigned char read_byte(unsigned short address) override
    {
        if (address >= 0x6000) {
            switch (address >> 12) {
                case 0x6:
                case 0x7:
                    if (rom_mode)
                        return *(prg_6000 + (address & 0x1FFF));
                    else
                        return *(ram + (address & 0x1FFF));
                    break;
                case 0x8:
                case 0x9:
                case 0xA:
                case 0xB:
                case 0xC:
                case 0xD:
                case 0xE:
                case 0xF:
                    if (!rom_mode && address >= 0xB800 && address < 0xD800)
                        return *(ram + 0x2000 + ((address - 0xB800) & 0x1FFF));
                    else
                        return c_mapper::read_byte(address);
            }
        }
        return c_mapper::read_byte(address);
    }

    void reset() override
    {
        rom_mode = 0;
        prg_6000 = pPrgRom;
        SetPrgBank32k(prgRomPageCount32k - 1);
    }


  private:
    int rom_mode;
    unsigned char *prg_6000;
    unsigned char *ram;
};

} //namespace nes
