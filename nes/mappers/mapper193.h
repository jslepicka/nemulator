#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper193 : public c_mapper, register_class<nes_mapper_registry, c_mapper193>
{
public:
    c_mapper193();
    ~c_mapper193() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 193,
                .constructor = []() { return std::make_unique<c_mapper193>(); },
            },
        };
    }
};

} //namespace nes
