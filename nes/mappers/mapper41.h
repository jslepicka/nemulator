#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper41 : public c_mapper, register_mapper<c_mapper41>
{
public:
    c_mapper41();
    ~c_mapper41() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 41,
                .name = "Caltron 6-in-1",
                .constructor = []() { return std::make_unique<c_mapper41>(); },
            }
        };
    }
private:
    int enable_reg2;
    int chr;
};

} //namespace nes
