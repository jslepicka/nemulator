#pragma once
#include "mapper4.h"

namespace nes {

class c_mapper95 : public c_mapper4, register_class<nes_mapper_registry, c_mapper95>
{
public:
    c_mapper95();
    ~c_mapper95() {};
    void write_byte(unsigned short address, unsigned char value);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 95,
                .constructor = []() { return std::make_unique<c_mapper95>(); },
            },
        };
    }

  protected:
    void Sync();
};

} //namespace nes
