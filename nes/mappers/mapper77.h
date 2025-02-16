#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper77 : public c_mapper, register_mapper<c_mapper77>
{
public:
    c_mapper77();
    ~c_mapper77();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 77,
                .constructor = []() { return std::make_unique<c_mapper77>(); },
            },
        };
    }

  private:
    unsigned char *chr_ram;
};

} //namespace nes
