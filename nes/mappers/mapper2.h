#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper2 : public c_mapper, register_mapper<c_mapper2>
{
public:
    c_mapper2();
    ~c_mapper2();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 2,
                .name = "UxROM",
                .constructor = []() { return std::make_unique<c_mapper2>(); },
            }
        };
    }
};

} //namespace nes
