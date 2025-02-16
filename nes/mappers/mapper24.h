#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper24 : public c_mapper, register_mapper<c_mapper24>
{
public:
    c_mapper24(int submapper = 0);
    ~c_mapper24() {};
    void write_byte(unsigned short address, unsigned char value);
    void clock(int cycles);
    void reset();
    float mix_audio(float sample);
    static std::vector<c_mapper::s_mapper_info> get_mapper_info()
    {
        return {
            {
                .number = 24,
                .name = "VRC6",
                .constructor = []() { return std::make_unique<c_mapper24>(); },
            },
            {
                .number = 26,
                .name = "VRC6",
                .constructor = []() { return std::make_unique<c_mapper24>(1); },
            },
        };
    }
private:
    int ticks;
    int irq_counter;
    int irq_asserted;
    int irq_control;
    int irq_reload;
    int irq_scaler;
    float audio_out;

    void mix();

    int freq_control;

    class c_channel
    {
    public:
        c_channel() {};
        virtual ~c_channel() {};
        virtual void clock();
        virtual int get_output();
        virtual void reset();
        virtual void set_volume(int val);
        virtual void set_timer_low(int val);
        virtual void set_timer_high(int val);
    protected:
        int volume;
        int timer_low;
        int timer_high;
        virtual void clock_channel() = 0;
        int timer;
        int output;
    };

    class c_pulse : public c_channel
    {
    public:
        c_pulse() {};
        ~c_pulse() {};
        void reset();
        void set_timer_high(int val);
    private:
        void clock_channel();
        int step;
    } pulse1, pulse2;

    class c_saw : public c_channel
    {
    public:
        c_saw() {};
        ~c_saw() {};
        void reset();
        void set_timer_high(int val);
    private:
        int step;
        int accum;
        virtual void clock_channel();
    } saw;



};

} //namespace nes
