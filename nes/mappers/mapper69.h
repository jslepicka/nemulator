#pragma once
#include "..\mapper.h"

class c_mapper69 :
    public c_mapper
{
public:
    c_mapper69();
    ~c_mapper69();
    void WriteByte(unsigned short address, unsigned char value);
    unsigned char ReadByte(unsigned short address);
    void reset();
    void clock(int cycles);
    float mix_audio(float sample);
private:
    int tick;
    int audio_tick;
    int command;
    int prg_mode;
    unsigned char *prg_6k;
    bool ram_enabled;
    unsigned short irq_counter;
    unsigned char irq_control;
    unsigned int audio_register_select;
    unsigned int audio_register[16];
    struct s_square
    {
        int enabled;
        int target_period;
        int current_period;
        int phase_count;
        int phase;
        float volume;
    } squares[3];
    static const float vol_table[16];
    int squares_enabled;
};