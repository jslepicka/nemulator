#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper94 : public c_mapper, register_class<c_mapper_registry, c_mapper94>
{
public:
    c_mapper94();
    ~c_mapper94();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 94,
                .constructor = []() { return std::make_unique<c_mapper94>(); },
            },
        };
    }
};

} //namespace nes
