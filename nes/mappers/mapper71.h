#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper71 : public c_mapper, register_mapper<c_mapper71>
{
public:
    c_mapper71();
    ~c_mapper71() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    int enable_mirroring_control;
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 71,
                .constructor = []() { return std::make_unique<c_mapper71>(); },
            },
        };
    }
};

} //namespace nes
