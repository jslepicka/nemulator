#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper9 : public c_mapper, register_class<c_mapper_registry, c_mapper9>
{
public:
    c_mapper9();
    virtual ~c_mapper9();
    virtual void write_byte(unsigned short address, unsigned char value);
    virtual void reset();
    void mmc2_latch(int address);
    unsigned char read_chr(unsigned short address);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 9,
                .name = "MMC2",
                .constructor = []() { return std::make_unique<c_mapper9>(); },
            }
        };
    }
private:
    int latch0;
    int latch1;
    int latch0FD;
    int latch0FE;
    int latch1FD;
    int latch1FE;
    int latch_buffer[2];
    unsigned int latch_buffer_head;
    unsigned int latch_buffer_tail;
};

} //namespace nes
