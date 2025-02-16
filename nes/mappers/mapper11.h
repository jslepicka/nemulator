#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper11 : public c_mapper, register_class<c_mapper_registry, c_mapper11>
{
public:
    c_mapper11();
    ~c_mapper11();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 11,
                .name = "Color Dreams",
                .constructor = []() { return std::make_unique<c_mapper11>(); },
            }
        };
    }
};

} //namespace nes
