#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper92 : public c_mapper, register_class<nes_mapper_registry, c_mapper92>
{
public:
    c_mapper92();
    ~c_mapper92() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 92,
                .constructor = []() { return std::make_unique<c_mapper92>(); },
            },
        };
    }

  private:
    int latch;
};

} //namespace nes
