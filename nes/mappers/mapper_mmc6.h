#pragma once
#include "mapper4.h"

namespace nes {

class c_mapper_mmc6 : public c_mapper4, register_class<nes_mapper_registry, c_mapper_mmc6>
{
public:
    c_mapper_mmc6();
    ~c_mapper_mmc6();
    void write_byte(unsigned short addres, unsigned char value);
    unsigned char read_byte(unsigned short address);
    void reset();
    int load_image();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 0x100,
                .name = "MMC6",
                .constructor = []() { return std::make_unique<c_mapper_mmc6>(); },
            },
        };
    }

  protected:
    int wram_enable;
    int wram_control;
};

} //namespace nes
