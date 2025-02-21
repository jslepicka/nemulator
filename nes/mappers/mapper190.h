#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper190 : public c_mapper, register_class<nes_mapper_registry, c_mapper190>
{
public:
    c_mapper190();
    ~c_mapper190();
    void write_byte(unsigned short address, unsigned char value);
    unsigned char read_byte(unsigned short address);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 190,
                .constructor = []() { return std::make_unique<c_mapper190>(); },
            },
        };
    }

  private:
    unsigned char *ram;
};

} //namespace nes
