#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper66 : public c_mapper, register_class<c_mapper_registry, c_mapper66>
{
public:
    c_mapper66();
    ~c_mapper66();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 66,
                .name = "GxROM",
                .constructor = []() { return std::make_unique<c_mapper66>(); },
            }
        };
    }
};

} //namespace nes
