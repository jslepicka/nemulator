#pragma once
#include "mapper4.h"

namespace nes {

class c_mapper44 : public c_mapper4, register_class<c_mapper_registry, c_mapper44>
{
public:
    c_mapper44();
    ~c_mapper44() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 44,
                .constructor = []() { return std::make_unique<c_mapper44>(); },
            }
        };
    }
};

} //namespace nes
