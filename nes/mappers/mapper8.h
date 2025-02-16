#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper8 : public c_mapper, register_class<c_mapper_registry, c_mapper8>
{
public:
    c_mapper8();
    ~c_mapper8();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 8,
                .name = "FFE F3xxx",
                .constructor = []() { return std::make_unique<c_mapper8>(); },
            }
        };
    }
};

} //namespace nes
