#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper76 : public c_mapper, register_mapper<c_mapper76>
{
public:
    c_mapper76();
    ~c_mapper76() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 76,
                .constructor = []() { return std::make_unique<c_mapper76>(); },
            },
        };
    }

  private:
    void sync();
    int prg_mode;
    int command;
};

} //namespace nes
