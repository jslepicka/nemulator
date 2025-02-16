#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper7 : public c_mapper, register_class<c_mapper_registry, c_mapper7>
{
public:
    c_mapper7();
    ~c_mapper7();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 7,
                .name = "AxROM",
                .constructor = []() { return std::make_unique<c_mapper7>(); },
            }
        };
    }
};

} //namespace nes
