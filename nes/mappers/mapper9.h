#pragma once
#include "..\mapper.h"

class c_mapper9 :
    public c_mapper
{
public:
    c_mapper9();
    virtual ~c_mapper9();
    virtual void WriteByte(unsigned short address, unsigned char value);
    virtual void reset();
    void mmc2_latch(int address);
    unsigned char ReadChrRom(unsigned short address);
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