#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper69 : public c_mapper, register_class<nes_mapper_registry, c_mapper69>
{
public:
    c_mapper69();
    ~c_mapper69();
    void write_byte(unsigned short address, unsigned char value);
    unsigned char read_byte(unsigned short address);
    void reset();
    void clock(int cycles);
    float mix_audio(float sample);
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 69,
                .name = "FME-7",
                .constructor = []() { return std::make_unique<c_mapper69>(); },
            }
        };
    }
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

} //namespace nes
