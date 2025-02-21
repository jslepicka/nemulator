#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper32 : public c_mapper, register_class<nes_mapper_registry, c_mapper32>
{
public:
    c_mapper32();
    ~c_mapper32() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 32,
                .constructor = []() { return std::make_unique<c_mapper32>(); },
            }
        };
    }
private:
    int prg_reg0;
    int prg_reg1;
    int prg_mode;
    void sync();
};

} //namespace nes
