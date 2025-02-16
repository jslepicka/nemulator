#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper97 : public c_mapper, register_class<c_mapper_registry, c_mapper97>
{
public:
    c_mapper97();
    ~c_mapper97() {};
    void reset();
    void write_byte(unsigned short address, unsigned char value);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 97,
                .constructor = []() { return std::make_unique<c_mapper97>(); },
            },
        };
    }
};

} //namespace nes
