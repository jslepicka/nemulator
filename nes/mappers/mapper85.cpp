#include "mapper85.h"
#include "..\cpu.h"
#include <math.h>
#include "vrc7_audio.h"

namespace nes {

const int c_mapper85::instruments[16][8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x03, 0x21, 0x04, 0x06, 0x8D, 0xF2, 0x42, 0x17,  // instrument 1
    0x13, 0x41, 0x05, 0x0E, 0x99, 0x96, 0x63, 0x12,  // instrument 2
    0x31, 0x11, 0x10, 0x0A, 0xF0, 0x9C, 0x32, 0x02,  // instrument 3
    0x21, 0x61, 0x1D, 0x07, 0x9F, 0x64, 0x20, 0x27,  // instrument 4
    0x22, 0x21, 0x1E, 0x06, 0xF0, 0x76, 0x08, 0x28,  // instrument 5
    0x02, 0x01, 0x06, 0x00, 0xF0, 0xF2, 0x03, 0x95,  // instrument 6
    0x21, 0x61, 0x1C, 0x07, 0x82, 0x81, 0x16, 0x07,  // instrument 7
    0x23, 0x21, 0x1A, 0x17, 0xEF, 0x82, 0x25, 0x15,  // instrument 8
    0x25, 0x11, 0x1F, 0x00, 0x86, 0x41, 0x20, 0x11,  // instrument 9
    0x85, 0x01, 0x1F, 0x0F, 0xE4, 0xA2, 0x11, 0x12,  // instrument A
    0x07, 0xC1, 0x2B, 0x45, 0xB4, 0xF1, 0x24, 0xF4,  // instrument B
    0x61, 0x23, 0x11, 0x06, 0x96, 0x96, 0x13, 0x16,  // instrument C
    0x01, 0x02, 0xD3, 0x05, 0x82, 0xA2, 0x31, 0x51,  // instrument D
    0x61, 0x22, 0x0D, 0x02, 0xC3, 0x7F, 0x24, 0x05,  // instrument E
    0x21, 0x62, 0x0E, 0x00, 0xA1, 0xA0, 0x44, 0x17,  // instrument F
};

const float c_mapper85::pi = 3.1415926535897932384626433832795f;

c_mapper85::c_mapper85()
{
    //Lagrange Point (J)
    //Tiny Toon Adventures 2 (J)
    mapperName = "VRC7";

    for (int i = 0; i < 256; i++)
    {
        
        float sinx = (float)sin(pi*i / 256.0f);
        if (!sinx)
            sin_table[i] = (1 << 23);
        else
        {
            sinx = linear2db(sinx);
            if (sinx >(1 << 23))
                sin_table[i] = (1 << 23);
            else
                sin_table[i] = (int)sinx;
        }
        //sin_table[i] = linear2db(sin(pi * i / 256.0f));
    }

    expansion_audio = 1;
    build_luts();
}

void c_mapper85::reset()
{
    SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
    irq_counter = 0;
    irq_asserted = 0;
    irq_control = 0;
    irq_reload = 0;
    irq_scaler = 0;
    audio_register_select = 0;
    ticks = 0;
    audio_ticks = 0;
    memset(phase, 0, sizeof(phase));
    memset(phase_secondary, 0, sizeof(phase_secondary));
    memset(channel_regs, 0, sizeof(channel_regs));
    memset(egc, 0, sizeof(egc));
    memset(envelope_state, 0, sizeof(envelope_state));
    audio_output = 0.0f;
    last_audio_output = 0.0f;
    am_counter = 0;
    fm_counter = 0;
    for (int i = 0; i < 12; i++)
        output[i] = 0;
    va.Reset(true);
}

void c_mapper85::WriteByte(unsigned short address, unsigned char value)
{
    if (address >= 0x8000)
    {
        switch (address >> 12)
        {
        case 0x8:
            if (address & 0x18)
                SetPrgBank8k(PRG_A000, value);
            else
                SetPrgBank8k(PRG_8000, value);
            break;
        case 0x9:
            if (address & 0x18)
            {
                va.Write(address, value);
                if (address == 0x9010) // audio register select
                {
                    audio_register_select = value;
                }
                else if (address == 0x9030) //audio register write
                {
                    audio_register_write(audio_register_select, value);
                }
            }
            else
                SetPrgBank8k(PRG_C000, value);
            break;
        case 0xA:
        case 0xB:
        case 0xC:
        case 0xD:
        {
            int bank = ((address >> 12) - 0xA) * 2;
            if (address & 0x18)
                bank++;
            SetChrBank1k(bank, value);
        }
            break;
        case 0xE:
            if (address & 0x18)
            {
                irq_reload = value;
            }
            else
            {
                switch (value & 0x3)
                {
                case 0:
                    set_mirroring(MIRRORING_VERTICAL);
                    break;
                case 1:
                    set_mirroring(MIRRORING_HORIZONTAL);
                    break;
                case 2:
                    set_mirroring(MIRRORING_ONESCREEN_LOW);
                    break;
                case 3:
                    set_mirroring(MIRRORING_ONESCREEN_HIGH);
                    break;
                }
            }
            break;
        case 0xF:
            if (address & 0x18)
            {
                if (irq_asserted)
                {
                    cpu->clear_irq();
                    irq_asserted = 0;
                }
                if (irq_control & 0x1)
                    irq_control |= 0x2;
                else
                    irq_control &= ~0x2;
            }
            else
            {
                irq_control = value & 0x7;
                if (irq_control & 0x2)
                {
                    irq_scaler = 0;
                    irq_counter = irq_reload;
                }
                if (irq_asserted)
                {
                    cpu->clear_irq();
                    irq_asserted = 0;
                }
            }
            break;
        }
    }
    else
    {
        c_mapper::WriteByte(address, value);
    }
}

void c_mapper85::clock(int cycles)
{
    ticks += cycles;
    while (ticks > 2)
    {
        if ((irq_control & 0x02) && ((irq_control & 0x04) || ((irq_scaler += 3) >= 341)))
        {
            if (!(irq_control & 0x04))
            {
                irq_scaler -= 341;
            }
            if (irq_counter == 0xFF)
            {
                irq_counter = irq_reload;
                if (!irq_asserted)
                {
                    cpu->execute_irq();
                    irq_asserted = 1;
                }
            }
            else
                irq_counter++;
        }
        ticks -= 3;
        if (++audio_ticks == 36)
        {
            va.clock(0);
            //clock_audio();
            audio_ticks = 0;
        }
    }

    //ticks += cycles;
    //while (ticks >= 108)
    //{
    //    clock_audio();
    //    ticks -= 108;
    //}
}

void c_mapper85::audio_register_write(int audio_register, int value)
{
    //get key_state
    int channel = audio_register & 0x7;
    int prev_key_state = channel_regs[channel][2] & 0x10;

    if (audio_register & 0x30)
    {
        channel_regs[audio_register & 0x7][((audio_register & 0x30) >> 4)] = value;
    }
    else
    {
        custom_instrument[audio_register & 0x7] = value;
    }

    if (audio_register & 0x30)
    {
        int new_key_state = channel_regs[channel][2] & 0x10;
        if (prev_key_state ^ new_key_state)
        {
            if (new_key_state)
            {
                for (int slot = channel * 2; slot <= channel * 2 + 1; slot++)
                {
                    key_on(slot);
                }
            }
            else
            {
                for (int slot = channel * 2; slot <= channel * 2 + 1; slot++)
                {
                    key_off(slot);
                }
            }
        }
    }

}

void c_mapper85::clock_audio()
{
    static int multiplier[] = { 1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30 };
    audio_output = 0.0f;
    const int *inst;
    am_counter = (am_counter + 78) & 0xFFFFF;
    fm_counter = (fm_counter + 105) & 0xFFFFF;

    //float am_output = (1.0f + sin(2.0f*pi*am_counter / (1 << 20))) * db(.6f);
    //float fm_output = pow(2.0f, 13.75f / 1200.0f * sin(2.0f*pi*fm_counter / (1 << 20)));

    uint32_t am_output = AmLut[(am_counter >> AmLutShift) & AmLutMask];
    double fm_output = FmLut[(fm_counter >> FmLutShift) & FmLutMask];

    for (int channel = 0; channel < 6; channel++)
    {
        int f = channel_regs[channel][1] | ((channel_regs[channel][2] & 0x1) << 8);
        int b = (channel_regs[channel][2] >> 1) & 0x7;
        int instrument = channel_regs[channel][3] >> 4;

        if (channel == 0)
            inst = custom_instrument;
        else
            inst = instruments[instrument];


        for (int slot = channel*2; slot <= (channel*2)+1; slot++)
        {
            //if (envelope_state[slot] == ENV_IDLE)
            //    continue;
            int is_carrier = slot & 1;
            int m = 0;
            m = multiplier[inst[is_carrier] & 0xF];
            uint32_t freq_rate = f * (1 << b) * m / 2;

            //int vibrato = inst[is_carrier] & 0x40 ? fm_output : 1;
            //phase[slot] += (f * (1 << b) * m * vibrato / 2); // * m * v / 2
            //phase[slot] &= 0x3FFFF;
            /*uint32_t mod = 0;*/
            if (inst[is_carrier] & 0x40)
                phase[slot] += (unsigned int)(freq_rate / 2 * fm_output);
            else
                phase[slot] += freq_rate / 2;
            if (is_carrier)
            {
                phase_secondary[slot] = (phase[slot] + ((unsigned int)output[slot - 1]/* & 0x3FFFF*/)) & 0x3FFFF;
            }
            else
            {
                int feedback = 0;
                if (inst[3] & 0x7)
                {
                    feedback = output[slot] >> (8 - (inst[3] & 0x7));
                }
                phase_secondary[slot] = (phase[slot] + feedback) & 0x3FFFF;

            }
            uint32_t env = clock_envelope(slot, inst);
            uint32_t mod = phase_secondary[slot];
            env += HalfSineLut[(mod >> PhaseShift) & PhaseMask];

            //output[slot] = env;
            if (env < MaxAtten)
            {
                int out = LinearLut[env >> LinearLutShift];
                if (mod & (1 << 17))
                {
                    if (inst[3] & (0x8 << is_carrier))
                    {
                        out = 0;
                    }
                    else
                    {
                        out = -out;
                    }
                }
                if (is_carrier && out != 0)
                {
                    float f = (float)(out / 1048576.0);
                    if (f > 1.0)
                    {
                        int x = 1;
                    }
                    f *= .5;
                    f += .5;
                    audio_output += f;
                }
                output[slot] = out;
            }
            else
            {
                output[slot] = 0;
            }
        }


    }
    if (audio_output > 6.0f)
    {
        int x = 1;
    }
    audio_output /= 6.0f;
    //audio_output += last_audio_output;
    //audio_output /= 2.0f;

}

float c_mapper85::mix_audio(float sample)
{
    float v = (float)(va.audio_out / 50000.0);
    return (sample *.5f) + (v * .5f);
}

float c_mapper85::linear2db(float in)
{
    return (float)(-20.0f * log10(in) * ((1 << 23) / 48.0f));
}

float c_mapper85::db2linear(float in)
{
    int outscale = (1 << 20);
    double inscale = (1 << 23) / 48.0 / (1 << 3);
    double lin = pow(10.0, (in / -20.0 / inscale));
    //return lin * outscale;
    return (float)pow(10.0f, (in / -20.0f / ((1 << 23) / 48.0f)));
}

uint32_t c_mapper85::clock_envelope(int slot, const int *inst)
{
    int is_carrier = slot & 0x1;
    //slot attack rate
    //int r = is_carrier ? inst[5] >> 4 : inst[4] >> 4;
    int r = 0;
    int rks = 0;
    int rh = 0;
    int rl = 0;
    int bf = channel_regs[slot & (~1)][2] & 0xF;
    int kb = inst[is_carrier] & 0x10 ? bf : bf >> 2;
    int sustain_level = 0;
    uint32_t out = 0;

    switch (envelope_state[slot])
    {
    case ENV_IDLE:
        return (1 << 23);
        break;
    case ENV_ATTACK:
        out = AttackLut[(egc[slot] >> AttackLutShift) & AttackLutMask];
        r = is_carrier ? inst[5] >> 4 : inst[4] >> 4;
        if (r != 0)
        {
            rks = r * 4 + kb;//fix: add kb +
            rh = rks >> 2;
            if (rh > 15)
                rh = 15;
            rl = rks & 3;
            egc[slot] += ((12 * (rl + 4)) << rh);
            if (egc[slot] >= (1 << 23))
            {
                egc[slot] = 0;
                envelope_state[slot] = ENV_DECAY;
            }
        }
        //out = db(48.0f) - (db(48.0f) * log(float(egc[slot] == 0 ? 1 : egc[slot])) / log((float)(1 << 23)));
        //return (int)out;
        return out;
        break;
    case ENV_DECAY:
        out = egc[slot];
        r = is_carrier ? inst[5] & 0xF : inst[4] & 0xF;
        rks = r * 4 + kb;
        rh = rks >> 2;
        if (rh > 15)
            rh = 15;
        rl = rks & 3;
        egc[slot] += ((rl + 4) << (rh - 1));
        sustain_level = is_carrier ? inst[7] >> 4 : inst[6] >> 4;
        sustain_level = (dB(3) * sustain_level/* * (1 << 23) / 48*/);
        if (egc[slot] >= sustain_level)
        {
            egc[slot] = sustain_level;
            envelope_state[slot] = ENV_SUSTAIN;
        }
        return out;
        break;
    case ENV_SUSTAIN:
        out = egc[slot];
        //fix: percussive
        r = 0;

        if (is_carrier)
        {
            if (!(inst[1] & 0x20))
                r = inst[7] >> 4;
        }
        else if (!(inst[0] & 0x20))
        {
            r = inst[6] >> 4;
        }
        rks = r * 4 + kb;
        rh = rks >> 2;
        if (rh > 15)
            rh = 15;
        rl = rks & 3;
        egc[slot] += (rl + 4) << (rh - 1);
        if (egc[slot] >= (1 << 23))
        {
            egc[slot] = (1 << 23);
            envelope_state[slot] = ENV_IDLE;
        }
        return out;
        break;
    case ENV_RELEASE:
        out = egc[slot];
        if (channel_regs[slot >> 1][2] & 0x20)
        {
            r = 5;
        }
        else
        {
            r = 7;
        }
        rks = r * 4 + kb;
        rh = rks >> 2;
        if (rh > 15)
            rh = 15;
        rl = rks & 3;
        egc[slot] += (rl + 4) << (rh - 1);
        if (egc[slot] >= (1 << 23))
        {
            egc[slot] = (1 << 23);
            envelope_state[slot] = ENV_IDLE;
        }
        return out;
        break;
    default:
        return (1 << 23);
    }

    return (1 << 23);
}

void c_mapper85::key_on(int slot)
{
    egc[slot] = 0;
    phase[slot] = 0;
    envelope_state[slot] = ENV_ATTACK;
}

void c_mapper85::key_off(int slot)
{
    //if (envelope_state[slot] = ENV_ATTACK)
    //    egc[slot] = output[slot];
    envelope_state[slot] = ENV_RELEASE;
}

//int c_mapper85::db(float db)
//{
//    return (int)(db * (float)(1 << 23) / 48.0f);
//}

void c_mapper85::build_luts()
{
    static const double pi = 3.1415926535897932384626433832795;

    // Build the half sine lut
    for (int i = 0; i < HalfSineWidth; ++i)
    {
        static const double scale = MaxAtten / 48.0;

        double sinx = sin(pi * i / HalfSineWidth);
        if (!sinx)       HalfSineLut[i] = MaxAtten;
        else
        {
            sinx = -20.0 * log10(sinx) * scale;
            if (sinx > MaxAtten) HalfSineLut[i] = MaxAtten;
            else                HalfSineLut[i] = static_cast<uint32_t>(sinx);
        }
    }

    // Build the Attack Rate Lut
    for (int i = 0; i < AttackLutWidth; ++i)
    {
        static const double baselog = log(static_cast<double>(MaxAtten >> AttackLutShift));

        AttackLut[i] = dB(48) - static_cast<uint32_t>(dB(48) * log(static_cast<double>(i)) / baselog);
    }

    // dB to Linear Lut
    for (int i = 0; i < LinearLutWidth; ++i)
    {
        static const int outscale = (1 << 20);              // channel output is 20 bits wide
        static const double inscale = MaxAtten / 48.0 / (1 << LinearLutShift);

        double lin = pow(10.0, (i / -20.0 / inscale));
        LinearLut[i] = static_cast<int>(lin * outscale);
    }

    // Am table
    for (int i = 0; i < AmLutWidth; ++i)
    {
        AmLut[i] = static_cast<uint32_t>((1.0 + sin(2 * pi * i / AmLutWidth)) * dB(0.6));
    }

    // Fm table
    for (int i = 0; i < FmLutWidth; ++i)
    {
        FmLut[i] = pow(2.0, 13.75 / 1200 * sin(2 * pi * i / FmLutWidth));
    }
}

inline int c_mapper85::dB(double dB)
{
    return static_cast<int>(dB * MaxAtten / 48.0);
}

} //namespace nes
