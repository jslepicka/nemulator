#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper232 : public c_mapper, register_class<c_mapper_registry, c_mapper232>
{
public:
    c_mapper232();
    ~c_mapper232() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 232,
                .constructor = []() { return std::make_unique<c_mapper232>(); },
            },
        };
    }

  private:
    int bank;
    int page;
    void sync();
};

} //namespace nes
