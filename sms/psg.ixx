module;

export module sms:psg;
import nemulator.std;

import dsp;

namespace sms
{

export class c_psg
{
  public:
    c_psg();
    ~c_psg();
    int get_buffer(const float **buf);
    void clear_buffer();
    void clock(int cycles);
    void write(int data);
    void reset();
    void set_audio_rate(double freq);
    void enable_mixer()
    {
        mixer_enabled = 1;
    }
    void disable_mixer()
    {
        mixer_enabled = 0;
    };

    float out;

  private:
    int available_cycles;
    int mixer_enabled = 0;
    int tick;
    int sample_count;
    short *sample_buffer;
    float vol_table[16];
    int vol[4];
    int tone[4];
    int counter[4];
    int output[4];
    int channel;
    int type;
    float sample_accumulator;
    int resampler_count;
    int noise_shift;
    int noise_mode;
    int noise_register;
    unsigned short lfsr;
    int lfsr_out;
    enum
    {
        TYPE_TONE = 0,
        TYPE_VOLUME = 0x10
    };
    
    using lpf_t =
        dsp::c_biquad4_t<0.5068508386611939f, 0.3307863473892212f, 0.1168005615472794f, 0.0055816280655563f,
                         -1.9496889114379883f, -1.9021773338317871f, -1.3770858049392700f, -1.9604763984680176f,
                         -1.9442052841186523f, -1.9171522855758667f, -1.8950747251510620f, -1.9676681756973267f,
                         0.9609073400497437f, 0.9271715879440308f, 0.8989855647087097f, 0.9881398081779480f>;
    using bpf_t = dsp::c_first_order_bandpass<>;
    using resampler_t = dsp::c_resampler2<1, lpf_t, bpf_t>;
    std::unique_ptr<resampler_t> resampler;
};

} //namespace sms