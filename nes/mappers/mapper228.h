#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper228 : public c_mapper, register_class<nes_mapper_registry, c_mapper228>
{
public:
    c_mapper228();
    ~c_mapper228();
    void write_byte(unsigned short address, unsigned char value);
    unsigned char read_byte(unsigned short address);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 228,
                .constructor = []() { return std::make_unique<c_mapper228>(); },
            },
        };
    }

  private:
    int regs[4];
};

} //namespace nes
