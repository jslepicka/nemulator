#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper146 : public c_mapper, register_class<nes_mapper_registry, c_mapper146>
{
public:
    c_mapper146();
    ~c_mapper146() {};
    void write_byte(unsigned short address, unsigned char value);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 146,
                .constructor = []() { return std::make_unique<c_mapper146>(); },
            },
        };
    }
};

} //namespace nes
