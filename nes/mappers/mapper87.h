#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper87 : public c_mapper, register_class<nes_mapper_registry, c_mapper87>
{
public:
    c_mapper87();
    ~c_mapper87() {};
    void write_byte(unsigned short address, unsigned char value);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 87,
                .constructor = []() { return std::make_unique<c_mapper87>(); },
            },
        };
    }
};

} //namespace nes
