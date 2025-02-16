#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper33 : public c_mapper, register_class<c_mapper_registry, c_mapper33>
{
public:
    c_mapper33();
    ~c_mapper33() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 33,
                .constructor = []() { return std::make_unique<c_mapper33>(); },
            }
        };
    }
};

} //namespace nes
