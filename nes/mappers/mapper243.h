#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper243 : public c_mapper, register_mapper<c_mapper243>
{
public:
    c_mapper243();
    ~c_mapper243() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 243,
                .constructor = []() { return std::make_unique<c_mapper243>(); },
            },
        };
    }

  private:
    int chr;
    int command;
    void sync_chr();
};

} //namespace nes
