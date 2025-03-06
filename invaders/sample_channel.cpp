module;
#include <cmath>
#include <memory>

module invaders:sample_channel;
import interpolate;

namespace invaders
{

c_sample_channel::c_sample_channel()
{
    loop = 0;
    len = 0;
    freq = 0;
    active = 0;
    step = 0.0;
    pos = 0.0;
}

void c_sample_channel::trigger()
{
    if (len) {
        active = 1;
        pos = 0.0;
    }
}

void c_sample_channel::stop()
{
    active = 0;
}

int c_sample_channel::is_active()
{
    return active;
}

float c_sample_channel::get_sample()
{
    int p1 = (int)pos;
    int p2 = p1 + 1;

    //linear interpolation
    double mu = pos - p1;
    float left_sample = (float)data[p1] / 32768.0f;
    float right_sample = 0.0;
    if (p2 <= len - 1) {
        right_sample = (float)data[p2] / 32768.0f;
    }
    pos += step;
    if (pos >= len) {
        if (loop) {
            pos = std::fmod(pos, (double)len);
        }
        else {
            stop();
        }
    }
    return interpolate::lerp(left_sample, right_sample, mu);
}

void c_sample_channel::load_wav(char *buf)
{
    struct s_fmt
    {
        uint32_t block_id;
        uint32_t block_size;
        uint16_t audio_format;
        uint16_t num_channels;
        uint32_t freq;
        uint32_t bytes_per_second;
        uint16_t bytes_per_block;
        uint16_t bits_per_sample;
    } *fmt;
    int p = 0;

    //skip over RIFF chunk
    p += 12;

    //read in fmt chunk
    fmt = (s_fmt *)&buf[12];
    p += 8 + fmt->block_size;

    //skip over fact chunk if it exists
    char fact[4] = {'f', 'a', 'c', 't'};
    if (memcmp(&buf[p], fact, 4) == 0) {
        int chunk_size = *(int *)&buf[p + 4];
        p += 8 + chunk_size;
    }

    char data_id[4] = {'d', 'a', 't', 'a'};
    if (memcmp(&buf[p], data_id, 4) == 0) {
        int chunk_size = *(int *)&buf[p + 4];
        int num_samples = chunk_size / 2;
        data = std::make_unique<int16_t[]>(num_samples);
        memcpy(data.get(), &buf[p + 8], chunk_size);
        len = num_samples;
        freq = fmt->freq;
    }
}

void c_sample_channel::set_playback_rate(double playback_freq)
{
    step = 1.0 / ((double)playback_freq / (double)freq);
}

void c_sample_channel::reset()
{
    active = 0;
    pos = 0.0;
}

} //namespace invaders