#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper80 : public c_mapper, register_class<nes_mapper_registry, c_mapper80>
{
public:
    c_mapper80();
    ~c_mapper80() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 80,
                .constructor = []() { return std::make_unique<c_mapper80>(); },
            },
        };
    }
};

} //namespace nes
