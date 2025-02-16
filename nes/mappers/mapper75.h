#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper75 : public c_mapper, register_mapper<c_mapper75>
{
public:
    c_mapper75();
    ~c_mapper75() {};
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 75,
                .name = "VRC-1",
                .constructor = []() { return std::make_unique<c_mapper75>(); },
            },
        };
    }

  private:
    void sync();
    int chr0, chr1;
};

} //namespace nes
