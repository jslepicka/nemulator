#include "pacman_psg.h"
#include <cstring>

c_pacman_psg::c_pacman_psg()
{
    sound_rom = new uint8_t[512];


    lpf = new c_biquad4({0.5067407488822937f, 0.3315044939517975f, 0.1230754777789116f, 0.0055863345041871f},
                        {-1.9293240308761597f, -1.8632467985153198f, -1.1734478473663330f, -1.9444166421890259f},
                        {-1.9303381443023682f, -1.9000983238220215f, -1.8756747245788574f, -1.9571375846862793f},
                        {0.9537956714630127f, 0.9141353368759155f, 0.8811427354812622f, 0.9859519004821777f});
    post_filter = new c_biquad(0.5648277401924133f, {1.0000000000000000f, 0.0000000000000000f, -1.0000000000000000f},
                               {1.0000000000000000f, -0.8659016489982605f, -0.1296554803848267f});
    resampler = new c_resampler(754560.0 / 48000.0, lpf, post_filter);
    reset();
}

c_pacman_psg::~c_pacman_psg()
{
    delete resampler;
    delete lpf;
    delete post_filter;
    delete[] sound_rom;
}

void c_pacman_psg::set_audio_rate(double freq)
{
    double x = 754560.0 / freq;
    resampler->set_m(x);
}

int c_pacman_psg::get_buffer(const short **buf)
{
    int num_samples = resampler->get_output_buf(buf);
    return num_samples;
}

void c_pacman_psg::clear_buffer()
{
    resampler->clear_buf();
}

void c_pacman_psg::write_byte(uint16_t address, uint8_t data)
{
    sound_ram[address - 0x40] = data & 0xF;
    int x = 1;
}

void c_pacman_psg::reset()
{
    memset(accumulator, 0, sizeof(uint32_t) * 3);
    muted = 1;
}

void c_pacman_psg::mute(int muted)
{
    this->muted = muted ? 1 : 0;
}

void c_pacman_psg::execute(int cycles)
{
    while (cycles-- > 0) {
        float sample = 0.0f;
        for (int channel = 0; channel < 3; channel++) {
            uint32_t channel_offset = channel * 5;
            uint32_t frequency = 
                (channel == 0 ? sound_ram[0x10] << 0 : 0) |
                sound_ram[0x11 + channel_offset] << 4 |
                sound_ram[0x12 + channel_offset] << 8 |
                sound_ram[0x13 + channel_offset] << 12 |
                sound_ram[0x14 + channel_offset] << 16;
            uint8_t volume = sound_ram[0x15 + channel_offset];
            uint8_t waveform = sound_ram[0x05 + channel_offset] & 0x7;
            accumulator[channel] = (accumulator[channel] + frequency) & 0xF'FFFF;
            uint8_t wave_index = (accumulator[channel] >> 15) & 0x1F;
            sample += muted ? 0.0f : (float)((sound_rom[waveform * 32 + wave_index] & 0xF) * volume);

        }

        sample /= 225.0f;
        //8x oversampled to ~768kHz
        for (int i = 0; i < 8; i++) {
            resampler->process(sample);
        }
    }
}