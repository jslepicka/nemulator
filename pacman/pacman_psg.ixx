module;

export module pacman:psg;
import nemulator.std;

import dsp;

namespace pacman
{
class c_pacman_psg
{
  public:
    c_pacman_psg();
    ~c_pacman_psg();
    void reset();
    void write_byte(uint16_t address, uint8_t data);
    void execute(int cycles);
    void set_audio_rate(double freq);
    int get_buffer(const float **buf);
    void clear_buffer();
    uint8_t *sound_rom;
    void mute(int muted);

    void enable_mixer()
    {
        mixer_enabled = 1;
    };
    void disable_mixer()
    {
        mixer_enabled = 0;
    };

  private:
    uint8_t sound_ram[32];
    uint32_t accumulator[3];

    using lpf_t = dsp::c_biquad4_t<0.5067391395568848f, 0.3314585983753204f, 0.1227479130029678f, 0.0055860662832856f,
                                 -1.9303771257400513f, -1.8652504682540894f, -1.1834223270416260f, -1.9452478885650635f,
                                 -1.9310271739959717f, -1.9009269475936890f, -1.8766038417816162f, -1.9576745033264160f,
                                 0.9541351795196533f, 0.9147564172744751f, 0.8819914460182190f, 0.9860565066337585f>;
    using bpf_t = dsp::c_first_order_bandpass<>;

    using resampler_t = dsp::c_resampler2<1, lpf_t, bpf_t>;

    std::unique_ptr<resampler_t> resampler;

    int32_t *sound_buffer;
    int muted;
    int mixer_enabled;

    //264 scanlines * 6 psg cycles/scanline * 60 fps * 8x oversampling
    static constexpr double audio_rate = 264.0 * 6.0 * 60.0 * 8.0;
};
} //namespace pacman