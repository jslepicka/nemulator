module;
#include "..\mapper.h"
export module nes_mapper.mapper13;

namespace nes
{

class c_mapper13 : public c_mapper, register_class<nes_mapper_registry, c_mapper13>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 13,
                .name = "NES-CPROM",
                .constructor = []() { return std::make_unique<c_mapper13>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            unsigned char *base = pChrRom + (value & 0x3) * 0x1000;
            chrBank[CHR_1000] = base;
            chrBank[CHR_1400] = base + 0x400;
            chrBank[CHR_1800] = base + 0x800;
            chrBank[CHR_1C00] = base + 0xC00;
        }
    }

    void reset() override
    {
        //TODO: Videomation has 16k CHR RAM, but mapper.cpp
        //only allocates 8k.  Should generalize that code instead
        //of reimplementing CHR RAM allocation and swapping here.
        if (chrRom != NULL) {
            delete[] chrRom;
            chrRom = new chrRomBank[2];
            memset(chrRom, 0, 16384);
            pChrRom = (unsigned char *)chrRom;
            for (int x = CHR_0000; x <= CHR_1C00; x++)
                chrBank[x] = pChrRom + 0x0400 * x;
        }
    }
};

} //namespace nes
