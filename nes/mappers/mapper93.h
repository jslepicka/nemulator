#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper93 : public c_mapper, register_mapper<c_mapper93>
{
public:
    c_mapper93();
    ~c_mapper93();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 93,
                .constructor = []() { return std::make_unique<c_mapper93>(); },
            },
        };
    }
};

} //namespace nes
