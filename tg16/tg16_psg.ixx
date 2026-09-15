module;
#include <cassert>
export module tg16:psg;
import nemulator.std;

import dsp;

namespace tg16
{

const std::array<float, 32> attenuation1_5 = []() {
    std::array<float, 32> out;
    for (int i = 0; i < 32; i++) {
        out[i] = std::pow(10.0, -((double)i * 1.5) / 20.0);
    }
    return out;
}();

class c_channel
{
  public:
    c_channel()
    {
        has_noise = false;
    }

    void reset()
    {
        freq = 0;
        amplitude = 0;
        l_amp = 0;
        r_amp = 0;
        wave_index = 0;
        dda_data = 0;
        counter = 0;
        output = 0;
        std::memset(wave_data, 0, sizeof(wave_data));
        lfsr = 1;
        lfsr_out = 0;
        noise_enabled = false;
        noise_freq = 0x1F;
        noise_counter = 32;
    }

    void write_wave_data(uint8_t value)
    {
        if (channel_on && dda) {
            wave_index = 0;
            dda_data = value;
        }
        else {
            wave_data[wave_index] = value;
            if (!channel_on && !dda) {
                wave_index = (wave_index + 1) & 0x1F;
            }
        }
    }

    void clock_lfsr()
    {
        noise_counter--;
        if (noise_counter == 0) {
            if (noise_freq == 0x1F) {
                noise_counter = 32;
            }
            else {
                noise_counter = ((~noise_freq) & 0x1F) * 64;
            }
            lfsr_out = lfsr & 1 ? 0x1F : 0;
            uint32_t feedback = ((lfsr >> 0) ^ (lfsr >> 1) ^ (lfsr >> 11) ^ (lfsr >> 12) ^ (lfsr >> 17)) & 0x01;
            lfsr >>= 1;
            lfsr |= (feedback << 17);
        }
    }

    void clock()
    {
        if (has_noise) {
            clock_lfsr();
        }
        counter = (counter - 1) & 0xFFF;
        if (counter == 0) {
            counter = freq;
            if (channel_on && !dda) {
                output = wave_data[wave_index];
                wave_index = (wave_index + 1) & 0x1F;
            }
        }
        if (channel_on && dda) {
            output = dda_data;
        }
        if (has_noise && noise_enabled) {
            output = lfsr_out;
        }
    }

    uint16_t freq;
    uint8_t amplitude;
    uint8_t l_amp;
    uint8_t r_amp;
    bool channel_on;
    bool dda;
    uint8_t wave_index;
    uint8_t wave_data[32];
    uint8_t dda_data;
    uint16_t counter;
    uint16_t noise_counter;
    uint8_t output;
    uint32_t lfsr;
    uint8_t lfsr_out;
    bool noise_enabled;
    uint8_t noise_freq;
    bool has_noise;
};

export class c_psg
{
  public:
    c_psg()
    {
        reset();
        resampler = resampler_t::create((float)(CLOCK_RATE / 48000.0));
        channel[4].has_noise = true;
        channel[5].has_noise = true;
    }

    int get_buffer(const float **buf)
    {
        return resampler->get_output_buf(buf);
    }

    void clear_buffer()
    {
        resampler->clear_buf();
    }

    void reset()
    {
        for (auto &c : channel) {
            c.reset();
        }
        selected_channel = 0;
        l_amp = 0;
        r_amp = 0;
        output = 0.0f;
        channel_divider = 0;
        output_divider = 0;
    }

    float attenuate_output(float val, uint8_t channel_amp, uint8_t channel_pan, uint8_t master_pan)
    {
        if (master_pan == 0 || channel_amp == 0 || channel_pan == 0) {
            return 0.0f;
        }
        else {
            auto atten = (31 - channel_amp) + (31 - (channel_pan * 2)) + (31 - (master_pan * 2));
            if (atten >= 30) {
                return 0.0f;
            }
            else {
                return val * attenuation1_5[atten];
            }
        }
    }

    void clock(int cycles)
    {
        for (int x = 0; x < cycles; x++) {
            for (auto &c : channel) {
                c.clock();
            }
            if (++output_divider == 4) {
                output_divider = 0;
                float output_l = 0.0f;
                float output_r = 0.0f;

                for (auto &c : channel) {
                    float cv = (float)c.output / 31.0f;
                    if (c.amplitude) {
                        output_l += attenuate_output(cv, c.amplitude, c.l_amp, l_amp);
                        output_r += attenuate_output(cv, c.amplitude, c.r_amp, r_amp);
                    }
                }
                output_l /= 6.0f;
                output_r /= 6.0f;
                resampler->process({output_l, output_r});
            }
        }

    }

    void write_byte(uint16_t address, uint8_t value)
    {
        assert(address < 10);
        //ods("psg: write %02X to address %04X\n", value, address);
        auto &c = channel[selected_channel];
        switch (address) {
            case 0:
                // channel select
                value &= 0x7;
               //ods("psg: select channel %d\n", value);
                selected_channel = value;
                break;
            case 1:
                // main amplitude level
                //ods("psg: set main amplitude %02X\n", value);
                l_amp = value >> 4;
                r_amp = value & 0xF;
                break;
            case 2:
                // freq low
                //ods("psg: set channel %d low freq to %02X\n", selected_channel, value);
                c.freq = (c.freq & ~0xFF) | value;
                break;
            case 3:
                // freq high
                value &= 0xF;
                //ods("psg: set channel %d high freq to %02X\n", selected_channel, value);
                c.freq = (c.freq & 0xFF) | (value << 8);
                break;
            case 4:
                // channel on, dda, channel amplitude
                //ods("psg: set channel %d R4 to %02X\n", selected_channel, value);
                c.channel_on = value & 0x80;
                c.dda = value & 0x40;
                if (!c.channel_on && c.dda) {
                    c.wave_index = 0;
                }
                else if (c.channel_on && c.dda) {
                    // should probably do this on clock or write
                    c.wave_index = 0;
                }
                c.amplitude = value & 0x1F;
                break;
            case 5:
                // l/r amplitude
                //ods("psg: set channel %d lr_amplitude to %02X\n", selected_channel, value);
                c.l_amp = value >> 4;
                c.r_amp = value & 0xF;
                break;
            case 6:
                // waveform
                value &= 0x1F;
                //ods("psg: write channel %d wave data %02X at index %d\n", selected_channel, value, c.wave_index);
                c.write_wave_data(value);
                break;
            case 7:
                // noise enable, freq
                //ods("psg: write channel %d noise data %02X\n", selected_channel, value);
                if (selected_channel > 3) {
                    c.noise_freq = value & 0x1F;
                    c.noise_enabled = value & 0x80;
                }
                break;
            case 8:
                // lfo freq
                //assert(value == 0);
                break;
            case 9:
                // lfo control
                //assert(value == 0);
                assert(!(value & 0x80));
                break;
            default:
                break;
        }
    }

    void set_audio_rate(double freq)
    {
        double x = CLOCK_RATE / freq;
        resampler->set_m((float)x);
    }

  private:
    c_channel channel[6];
    uint32_t selected_channel;
    uint8_t l_amp;
    uint8_t r_amp;
    float output;
    int channel_divider;
    int output_divider;

    constexpr static double CLOCK_RATE = 21477270.0 / 6.0 / 4.0;

    // 3.5MHz
    //using lpf_t =
    //    dsp::c_biquad4_t<0.5103541612625122f, 0.3330817222595215f, 0.1037740930914879f, 0.0055815293453634f,
    //                     -1.9968196153640747f, -1.9937456846237183f, -1.9544347524642944f, -1.9975079298019409f,
    //                     -1.9889812469482422f, -1.9805959463119507f, -1.9734793901443481f, -1.9957172870635986f,
    //                     0.9900443553924561f, 0.9812410473823547f, 0.9737335443496704f, 0.9970080256462097f>;

    //3.5MHz / 4
    using lpf_t =
        dsp::c_biquad4_t<0.5068490505218506f, 0.3307895660400391f, 0.1168397217988968f, 0.0055816541425884f,
                         -1.9495602846145630f, -1.9019303321838379f, -1.3757249116897583f, -1.9603750705718994f,
                         -1.9441134929656982f, -1.9170366525650024f, -1.8949410915374756f, -1.9676004648208618f,
                         0.9608581662178040f, 0.9270812869071960f, 0.8988617658615112f, 0.9881247282028198f>;
    using bpf_t = dsp::c_first_order_bandpass<>;
    using resampler_t = dsp::c_resampler<2, lpf_t, bpf_t>;
    std::unique_ptr<resampler_t> resampler;
};

} //namespace tg16