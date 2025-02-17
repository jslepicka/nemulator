#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper78 : public c_mapper, register_class<nes_mapper_registry, c_mapper78>
{
public:
    c_mapper78();
    ~c_mapper78() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 78,
                .constructor = []() { return std::make_unique<c_mapper78>(); },
            },
        };
    }

  private:
    int mirror_mode;
};

} //namespace nes
