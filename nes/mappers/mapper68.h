#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper68 : public c_mapper, register_mapper<c_mapper68>
{
public:
    c_mapper68();
    ~c_mapper68();
    void write_byte(unsigned short address, unsigned char value);
    void reset();
    void ppu_write(unsigned short address, unsigned char value);
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 68,
                .constructor = []() { return std::make_unique<c_mapper68>(); },
            }
        };
    }
private:
    int reg_c;
    int reg_d;
    int mirroring_mode;
    int nt_mode;
};

} //namespace nes
