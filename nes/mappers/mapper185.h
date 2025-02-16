#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper185 : public c_mapper, register_class<c_mapper_registry, c_mapper185>
{
public:
    c_mapper185();
    ~c_mapper185();
    void write_byte(unsigned short address, unsigned char value);
    unsigned char read_chr(unsigned short address);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 185,
                .constructor = []() { return std::make_unique<c_mapper185>(); },
            },
        };
    }
      private:
    int chr_protected;
};

} //namespace nes
