#pragma once
#include "..\mapper.h"
#include <cstdint>
#include "vrc7_audio.h"

namespace nes {

class c_mapper85 :
    public c_mapper
{
public:
    c_mapper85();
    ~c_mapper85() {};
    void WriteByte(unsigned short address, unsigned char value);
    void clock(int cycles);
    void reset();
    float mix_audio(float sample);
private:
    schpune::Vrc7Audio va;
    int ticks;
    int audio_ticks;
    int irq_counter;
    int irq_asserted;
    int irq_control;
    int irq_reload;
    int irq_scaler;
    int audio_register_select;

    unsigned char channel_regs[8][4];


    void audio_register_write(int audio_register, int value);
    void clock_audio();

    static const int instruments[16][8];
    int custom_instrument[8];
    unsigned int phase[12];
    unsigned int phase_secondary[12];
    int output[12];
    unsigned int egc[12];

    int sin_table[256];

    float audio_output;
    float last_audio_output;

    float linear2db(float in);
    float db2linear(float in);

    uint32_t clock_envelope(int slot, const int *inst);
    int envelope_state[12];

    enum ENVELOPE_STATES { ENV_IDLE = 0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };
    void key_on(int slot);
    void key_off(int slot);


    int am_counter;
    int fm_counter;

    static const float pi;

    void build_luts();


    //disch

    static const uint32_t      MaxAtten = (1 << 23);
    static const int       HalfSineBits = 10;
    static const int       HalfSineWidth = (1 << HalfSineBits);
    static const int       PhaseShift = 17 - HalfSineBits;
    static const int       PhaseMask = HalfSineWidth - 1;
    uint32_t HalfSineLut[HalfSineWidth];

    //// Attack rate table
    //  envelope generator is 23 bits wide.  Attack rate table can be any number
    // of bits up to that
    static const int       AttackLutBits = 8;      // no higher than 23
    static const int       AttackLutWidth = (1 << AttackLutBits);
    static const int       AttackLutShift = 23 - AttackLutBits;
    static const int       AttackLutMask = AttackLutWidth - 1;
    uint32_t AttackLut[AttackLutWidth];

    //// dB to Linear table
    //  envelope levels are 23 bits wide, based in dB
    //  channel output is 20 bits wide, linear
    static const int       LinearLutBits = 16;
    static const int       LinearLutWidth = (1 << LinearLutBits);
    static const int       LinearLutShift = 23 - LinearLutBits;
    int LinearLut[LinearLutWidth];

    //// AM / Tremolo table
    //  am rate accumulator is 20 bits wide
    static const int       AmLutBits = 8;
    static const int       AmLutWidth = (1 << AmLutBits);
    static const int       AmLutShift = 20 - AmLutBits;
    static const int       AmLutMask = (AmLutWidth - 1);
    uint32_t AmLut[AmLutWidth];

    //// FM / Vibratto table
    //  fm rate accumulator is 20 bits wide
    static const int       FmLutBits = 8;
    static const int       FmLutWidth = (1 << FmLutBits);
    static const int       FmLutShift = 20 - FmLutBits;
    static const int       FmLutMask = (FmLutWidth - 1);
    double FmLut[FmLutWidth];

    int dB(double dB);
};

} //namespace nes
