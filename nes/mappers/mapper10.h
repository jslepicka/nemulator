#pragma once
#include "mapper9.h"

namespace nes {

class c_mapper10: public c_mapper9, register_mapper<c_mapper10>
{
public:
    c_mapper10();
    ~c_mapper10() {};
    void reset();
    void write_byte(unsigned short address, unsigned char value);
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 10,
                .name = "MMC4",
                .constructor = []() { return std::make_unique<c_mapper10>(); },
            }
        };
    }
};

} //namespace nes
