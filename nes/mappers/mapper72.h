#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper72 : public c_mapper, register_mapper<c_mapper72>
{
public:
    c_mapper72();
    ~c_mapper72() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 72,
                .constructor = []() { return std::make_unique<c_mapper72>(); },
            },
        };
    }

  private:
    int latch;
};

} //namespace nes
