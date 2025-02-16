#pragma once
#include "mapper4.h"

namespace nes {

class c_mapper115 : public c_mapper4, register_class<c_mapper_registry, c_mapper115>
{
public:
    c_mapper115();
    ~c_mapper115() {};
    void write_byte(unsigned short address, unsigned char value);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 115,
                .constructor = []() { return std::make_unique<c_mapper115>(); },
            },
        };
    }

  private:
    int reg1;
    void reset();
    void Sync();
};

} //namespace nes
