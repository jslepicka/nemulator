#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper15 : public c_mapper, register_mapper<c_mapper15>
{
public:
    c_mapper15();
    ~c_mapper15() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 15,
                .constructor = []() { return std::make_unique<c_mapper15>(); },
            }
        };
    }
private:
    void sync();
    int reg;
    int mode;
};

} //namespace nes
