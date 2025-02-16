#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper86 : public c_mapper, register_mapper<c_mapper86>
{
public:
    c_mapper86();
    ~c_mapper86() {};
    void write_byte(unsigned short address, unsigned char value);
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 86,
                .constructor = []() { return std::make_unique<c_mapper86>(); },
            },
        };
    }
};

} //namespace nes
