#pragma once
#include "mapper4.h"

namespace nes {

class c_mapper47 : public c_mapper4, register_class<nes_mapper_registry, c_mapper47>
{
public:
    c_mapper47();
    ~c_mapper47() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 47,
                .constructor = []() { return std::make_unique<c_mapper47>(); },
            }
        };
    }
};

} //namespace nes
