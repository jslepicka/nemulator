#pragma once
#include <cstdint>

namespace dsp
{
class c_biquad;
class c_biquad4;
class c_resampler;
} //namespace dsp

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
    int get_buffer(const short **buf);
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

    dsp::c_resampler *resampler;
    dsp::c_biquad4 *lpf;
    dsp::c_biquad *post_filter;
    int32_t *sound_buffer;
    int muted;
    int mixer_enabled;

    //264 scanlines * 6 psg cycles/scanline * 60 fps * 8x oversampling
    static constexpr double audio_rate = 264.0 * 6.0 * 60.0 * 8.0;
};
} //namespace pacman