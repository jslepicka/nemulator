#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper82 : public c_mapper, register_class<nes_mapper_registry, c_mapper82>
{
public:
    c_mapper82();
    ~c_mapper82() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 82,
                .constructor = []() { return std::make_unique<c_mapper82>(); },
            },
        };
    }

  private:
    void sync();
    int chr_mode;
    int chr[6];
};

} //namespace nes
