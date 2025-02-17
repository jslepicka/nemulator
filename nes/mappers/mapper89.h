#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper89 : public c_mapper, register_class<nes_mapper_registry, c_mapper89>
{
public:
    c_mapper89();
    ~c_mapper89() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 89,
                .constructor = []() { return std::make_unique<c_mapper89>(); },
            },
        };
    }
};

} //namespace nes
