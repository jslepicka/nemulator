#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper112 : public c_mapper, register_mapper<c_mapper112>
{
public:
    c_mapper112();
    ~c_mapper112() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 112,
                .constructor = []() { return std::make_unique<c_mapper112>(); },
            },
        };
    }

  private:
    int command;
};

} //namespace nes
