module;
#include "..\mapper.h"
export module nes_mapper.mapper119;

import nes_mapper.mapper4;

namespace nes
{

//High Speed, Pinbot

class c_mapper119 : public c_mapper4, register_class<nes_mapper_registry, c_mapper119>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 119,
                .name = "TQROM",
                .constructor = []() { return std::make_unique<c_mapper119>(); },
            },
        };
    }

    c_mapper119()
    {
        ram = new unsigned char[8192];
    }

    ~c_mapper119()
    {
        delete[] ram;
    }


  private:
    unsigned char *ram;

    void SetChrBank1k(int bank, int value) override
    {
        if (value & 0x40) {
            chrBank[bank] = ram + (((value & 0x3F) % 8) * 0x400);
        }
        else
            c_mapper::SetChrBank1k(bank, (value & 0x3F));
    }

    void write_chr(unsigned short address, unsigned char value) override
    {
        *(chrBank[(address >> 10) % 8] + (address & 0x3FF)) = value;
    }
};

} //namespace nes
