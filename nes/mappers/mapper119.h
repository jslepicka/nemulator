#pragma once
#include "mapper4.h"

namespace nes {

class c_mapper119 : public c_mapper4, register_mapper<c_mapper119>
{
public:
    c_mapper119();
    ~c_mapper119();
    void SetChrBank1k(int bank, int value);
    void write_chr(unsigned short address, unsigned char value);
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 119,
                .name = "TQROM",
                .constructor = []() { return std::make_unique<c_mapper119>(); },
            },
        };
    }

  private:
    unsigned char *ram;
};

} //namespace nes
