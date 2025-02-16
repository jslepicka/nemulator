#pragma once
#include "mapper4.h"

namespace nes {

class c_mapper118 : public c_mapper4, register_mapper<c_mapper118>
{
public:
    c_mapper118();
    ~c_mapper118() {};
    void write_byte(unsigned short address, unsigned char value);
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 118,
                .constructor = []() { return std::make_unique<c_mapper118>(); },
            },
        };
    }

  private:
    void Sync();
};

} //namespace nes
