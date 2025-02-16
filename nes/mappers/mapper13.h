#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper13 : public c_mapper, register_class<c_mapper_registry, c_mapper13>
{
public:
    c_mapper13();
    ~c_mapper13();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 13,
                .name = "NES-CPROM",
                .constructor = []() { return std::make_unique<c_mapper13>(); },
            }
        };
    }
};

} //namespace nes
