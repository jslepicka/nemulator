#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper103 : public c_mapper, register_mapper<c_mapper103>
{
public:
    c_mapper103();
    ~c_mapper103();
    void write_byte(unsigned short address, unsigned char value);
    unsigned char read_byte(unsigned short address);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 103,
                .name = "BTL 2708",
                .constructor = []() { return std::make_unique<c_mapper103>(); },
            },
        };
    }

  private:
    int rom_mode;
    unsigned char *prg_6000;
    unsigned char *ram;
};

} //namespace nes
