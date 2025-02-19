#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper70 : public c_mapper, register_class<nes_mapper_registry, c_mapper70>
{
public:
    c_mapper70();
    ~c_mapper70() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 70,
                .constructor = []() { return std::make_unique<c_mapper70>(); },
            },
        };
    }
};

} //namespace nes
